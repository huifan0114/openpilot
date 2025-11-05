## 變更紀錄

| 日期 | 版本 | 變更內容 |
|------|------|---------|
| 2025-11-05 | v3.2 | **✅ 更新**：Debug Log 自動循環機制改為建立新檔案（不再覆蓋）- 達到 500KB 時重新命名為 `frogpilot_debug.log.YYYYMMDD_HHMMSS`，保留最多 5 個舊檔案，避免資料遺失 - 更新 `frogpilot/common/frogpilot_debug_logger.py` |
| 2025-11-05 | v3.1 | **✅ 新增**：統一 Debug Log 系統（`frogpilot/common/frogpilot_debug_logger.py`）- 輸出到 `/data/frogpilot_debug.log` 純文字檔,可用 FTP 下載查看,所有我們的修改都應使用此 logger 而非 cloudlog - 詳見 `docs/14-debug-log-system.md` |
| 2025-11-05 | v3.0 | **✅ 實作**：SLC 簡化 - 只用 Map Data,自動設定 MAX SPEED,60km/h 差距檢查,UI 簡化只顯示 Upcoming 標誌,Map Data 速限四捨五入到 10 的倍數,無速限時保持上次值 - 修改 5 個檔案 - 詳見 `docs/13-slc-simplification.md` |
| 2025-11-02 | v2.8 | **新增**：速限控制修正 - VW Dashboard 無訊號問題解決方案（修改 `speed_limit_controller.py:245` 使 Dashboard 直接使用 Map Data） |
| 2025-11-02 | v2.7 | **✅ 研究**：ForceStops 停車機制深度分析（動態 v_cruise 降低策略、為什麼不需要實驗模式、與傳統路徑規劃停車的對比） |
| 2025-11-02 | v2.6 | **✅ 研究**：定速控制系統架構完整分析（VCruiseHelper vs FrogPilotVCruise 關係釐清、單向資料流、進程分離、職責分工） |
| 2025-11-01 | v2.5 | **✅ 實作**：油門啟動控制的安全初始速度（修改 `drive_helpers.py:160-164`）- 使用 max(當前速度, 40 kph) 作為初始巡航速度,配合油門恢復功能確保安全 |
| 2025-11-01 | v2.4 | **新增**：為什麼啟動後必須先按 SET 鍵才能啟用 OP 控制（v_cruise_initialized 機制、resumeBlocked 邏輯、3種情境流程圖、設計理念、跳過方法研究） |
| 2025-11-01 | v2.3 | **✅ 實作**：踩油門恢復控制功能（修改 `events.py:721` 新增 `ET.ENABLE` 到 `gasPressedOverride`）- 靜止煞車和 CANCEL 後可用油門恢復控制 |
| 2025-11-01 | v2.2 | **新增**：常見疑問解答（ENABLE/OVERRIDE 為何不衝突、NO_ENTRY 事件持續性分析、CANCEL 按住行為）；**優化**：詳細的狀態機邏輯說明、事件生命週期分析 |
| 2025-11-01 | v2.1 | **新增**：踩油門恢復控制功能完整實作方案（3種方案對比、推薦方案、實作步驟、測試場景、風險評估、進階選項） |
| 2025-11-01 | v2.0 | **新增**：控制恢復機制詳解（煞車/CANCEL 後能否用油門恢復）、事件類型與狀態機完整分析、設計理念說明；**更新**：煞車與油門差異表格新增 CANCEL 按鈕欄位 |
| 2025-10-23 | v1.9 | 實作：煞車自動回復功能修正版（依車速判斷：> 0.5 m/s 自動回復，≤ 0.5 m/s 需重新啟動），包含完整邏輯說明、測試情境、設計決策 |
| 2025-10-23 | v1.8 | 新增：mapd 執行檔與地圖資料詳細說明、params_memory 位置說明、手動測試 mapd 流程、完整故障診斷 |
| 2025-10-23 | v1.7 | 新增：車輛速限識別系統 (Dashboard Speed Limit)，包含 Toyota/Hyundai/Ford 實作、VW 未實作分析、speed_limit_filler.py 完整功能說明 |
| 2025-10-23 | v1.6 | 新增：FrogPilot UI 系統（彎道圖標、位置計算、實作案例）；實作：移動彎道速度控制圖標到 CEStatus 位置 |
| 2025-10-23 | v1.5 | 新增：FrogPilot 導航與地圖系統（mapd 架構、MapSpeedLimit/RoadName PARAMS）、CEM 啟動機制與關閉行為分析 |
| 2025-10-22 | v1.4 | 新增：控制系統詳細研究（煞車踏板修改、安全層邏輯、狀態機流程）|
| 2025-10-22 | v1.2 | 實作：時間持久化功能（`common/params.cc`, `system/timed.py`）|
| 2025-10-22 | v1.1 | 新增：知識庫使用指南、FrogPilot 模式系統、除錯與診斷、Git 工作流程 |
| 2025-10-22 | v1.0 | 初始版本，包含時間系統、PARAMS、電源管理研究成果 |

---

**文件維護指南**：

1. 每次深入研究後，將結果整合到對應章節
2. 保持實作案例的更新
3. 記錄所有 API 變更
4. 維護檔案位置索引的準確性
5. 添加實際遇到的問題和解決方案

**下次研究主題建議**：
- [ ] 車輛介面（selfdrive/car）架構
- [ ] UI 系統（Qt/QML）結構
- [ ] 模型推理流程（modeld）
- [ ] CAN 通訊機制
- [ ] 日誌上傳和下載流程
| 2025-11-02 | v2.9 | **新增**：SLC 速限控制邏輯深度解析（08-speed-limit-nav.md）- 詳細說明 SLC 如何使用 min() 只降速不加速、MAX SPEED 保持機制、多情境對比分析 |
