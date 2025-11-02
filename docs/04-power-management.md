# 電源管理

> **來源**: op-develop-guide.md
> **最後更新**: 2025-11-02

[← 返回主目錄](README-DEV.md)

---

## 電源管理

## 關機流程

### 完整流程圖

```
拔掉車輛電源
  ↓
內建電池供電開始
  ↓
hardwared.py 監測關機條件（每秒）
  ├─ 離線時間 > device_shutdown_time
  ├─ 電壓 < low_voltage_cutoff
  └─ 電池容量耗盡
  ↓
params.put_bool("DoShutdown", True)
  ↓
manager.py 偵測到 DoShutdown
  ↓
跳出 manager_thread() 主迴圈
  ↓
manager_cleanup()
  ├─ 發送 SIGINT 到所有進程
  ├─ 等待 5 秒
  └─ 未退出者發送 SIGKILL
  ↓
HARDWARE.shutdown()
  ↓
執行 "sudo poweroff"
```

### 關機觸發條件

**位置**: `system/hardware/power_monitoring.py:110-128`

```python
def should_shutdown(self, ignition, in_car, offroad_timestamp, started_seen, frogpilot_toggles):
    offroad_time = (now - offroad_timestamp)

    # 計算各種關機條件
    low_voltage_shutdown = (
        self.car_voltage_mV < (frogpilot_toggles.low_voltage_shutdown * 1e3)
        and offroad_time > 60
    )

    should_shutdown = False
    should_shutdown |= offroad_time > frogpilot_toggles.device_shutdown_time  # 時間到
    should_shutdown |= low_voltage_shutdown                                    # 低電壓
    should_shutdown |= (self.car_battery_capacity_uWh <= 0)                   # 沒電
    should_shutdown &= not ignition                                            # 未行駛
    should_shutdown &= (not self.params.get_bool("DisablePowerDown"))         # 允許關機
    should_shutdown &= in_car                                                  # 在車上
    should_shutdown &= offroad_time > DELAY_SHUTDOWN_TIME_S                   # 延遲 5 分鐘
    should_shutdown |= self.params.get_bool("ForcePowerDown")                 # 強制關機
    should_shutdown &= started_seen or (now > MIN_ON_TIME_S)                  # 最小開機時間

    return should_shutdown
```

### 關機常數

```python
# power_monitoring.py
DELAY_SHUTDOWN_TIME_S = 300              # 5 分鐘保護期
VOLTAGE_SHUTDOWN_MIN_OFFROAD_TIME_S = 60 # 低壓關機至少等 60 秒
MAX_TIME_OFFROAD_S = 30*3600             # 最大離線 30 小時
MIN_ON_TIME_S = 3600                     # 最小開機時間 1 小時
```

### 信號處理

**進程停止流程**：

```python
# process.py:112-142
def stop(self, sig=signal.SIGINT):
    if self.proc.exitcode is None:
        # 1. 發送信號（預設 SIGINT）
        self.signal(sig)

        # 2. 等待 5 秒
        join_process(self.proc, 5)

        # 3. 如果還沒退出，發送 SIGKILL
        if self.proc.exitcode is None:
            cloudlog.info(f"killing {self.name} with SIGKILL")
            self.signal(signal.SIGKILL)
            self.proc.join()
```

### Manager 退出處理

```python
# manager.py:223-242
signal.signal(signal.SIGTERM, lambda signum, frame: sys.exit(1))

try:
    manager_thread()
except Exception:
    traceback.print_exc()
    sentry.capture_exception()
finally:
    manager_cleanup()  # 確保清理

# 執行關機操作
params = Params()
if params.get_bool("DoUninstall"):
    uninstall_frogpilot()
elif params.get_bool("DoReboot"):
    HARDWARE.reboot()
elif params.get_bool("DoShutdown"):
    HARDWARE.shutdown()
```

---

## 電源監控

### PowerMonitoring 類別

**位置**: `system/hardware/power_monitoring.py`

### 電池容量計算

```python
class PowerMonitoring:
    CAR_BATTERY_CAPACITY_uWh = 30e6  # 30Wh
    CAR_CHARGING_RATE_W = 45         # 45W
    VBATT_PAUSE_CHARGING = 11.8      # 11.8V

    def calculate(self, voltage, ignition):
        # 低通濾波電壓
        self.car_voltage_mV = (
            (voltage * CAR_VOLTAGE_LOW_PASS_K) +
            (self.car_voltage_mV * (1 - CAR_VOLTAGE_LOW_PASS_K))
        )

        if ignition:
            # 行駛中：累積充電
            self.car_battery_capacity_uWh += (CAR_CHARGING_RATE_W * 1e6 * integration_time_h)
        else:
            # 離線中：減少電量
            current_power = HARDWARE.get_current_power_draw()
            power_used = (current_power * 1000000) * integration_time_h
            self.car_battery_capacity_uWh -= power_used

        # 每 10 秒保存一次
        if now - self.last_save_time >= 10:
            self.params.put_nonblocking("CarBatteryCapacity", str(int(self.car_battery_capacity_uWh)))
```

---

## Comma 3 電池管理

### Device Shutdown Timer

**參數**: `DeviceShutdown`（0-33）

**計算方式**：
```python
# frogpilot_variables.py:739
if device_shutdown_setting >= 4:
    device_shutdown_time = (device_shutdown_setting - 3) * 3600  # 小時
else:
    device_shutdown_time = device_shutdown_setting * (60 * 15)   # 15分鐘
```

**設定對照表**：

| 設定值 | 實際時間 |
|-------|---------|
| 0 | 立即關機 |
| 1 | 15 分鐘 |
| 2 | 30 分鐘 |
| 3 | 45 分鐘 |
| 4 | 1 小時 |
| 5 | 2 小時 |
| 9（預設）| 6 小時 |
| 33 | 30 小時 |

### Low Voltage Cutoff

**參數**: `LowVoltageShutdown`（11.8 - 12.5V）

**電壓對照**：
- 12.6V = 滿電（100%）
- 12.4V = 75% 電量
- 12.2V = 50% 電量
- 12.0V = 25% 電量
- 11.8V = 接近放電極限（預設值）

**說明**: 設定過低會導致電池過度放電，建議保持 11.8-12.0V 之間。

---

