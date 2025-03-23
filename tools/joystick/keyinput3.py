#!/usr/bin/env python
import math
import evdev
from openpilot.common.params import Params

mem_params = Params("/dev/shm/params")
params = Params()
mem_params.put_bool("KeyResume", False)
mem_params.put_bool("KeyCancel", False)
params.put_int("RoadtypeProfile", 1)
gap_prev = mem_params.get_int('KeyGap')
if gap_prev == 0:
    now_gap = 5
    mem_params.put_int('KeyGap', now_gap)

# def selekey(code, setspeed):
#     newsetspeed = setspeed
#     road_type = params.get_int("RoadtypeProfile")

#     # 速度對應表
#     speed_map = {
#         'A': {0: 40, 1: 20, 2: 20, 3: 40, 4: 50},
#         'B': {0: 50, 1: 30, 2: 50, 3: 80, 4: 100},
#         'C': {0: 60, 1: 40, 2: 60, 3: 90, 4: 110},
#         'D': {0: 70, 1: 50, 2: 70, 3: 100, 4: 120},
#     }

#     # 速度調整按鍵
#     if code in speed_map and road_type in speed_map[code]:
#         newsetspeed = speed_map[code][road_type]
#         mem_params.put_int('SpeedPrev', 0)
#         mem_params.put_bool("KeyResume", True)

#     # 導航與模式切換
#     elif code == 'E':
#         if mem_params.get_int("DetectSpeedLimit") != 0:
#             mem_params.put_bool('SpeedLimitChanged', True)
#         params.put_bool("TrafficMode", not params.get_bool("TrafficMode"))

#     elif code == 'F':
#         params.remove("NavDestination")

#     elif code == 'G':
#         params.put("NavDestination", "{\"latitude\": 24.2103369, \"longitude\": 120.5764539, \"place_name\": \"正英七街\"}")

#     elif code == 'H':
#         params.put("NavDestination", "{\"latitude\": 24.184996, \"longitude\": 120.603791, \"place_name\": \"台中榮總\"}")

#     # 設定相關按鍵
#     elif code == 'I':  # 切換 AutoACC
#         params.put_bool("AutoACC", not params.get_bool("AutoACC"))
#         mem_params.put_bool("FrogPilotTogglesUpdated", True)

#     elif code == 'J':  # 道路類型切換
#         newRoadtypeProfile = (road_type + 1) % 6
#         params.put_int("RoadtypeProfile", newRoadtypeProfile)
#         params.put_bool("AutoRoadtype", newRoadtypeProfile == 5)
#         mem_params.put_bool("FrogPilotTogglesUpdated", True)

#     elif code == 'K':  # 加速模式切換
#         params.put_int("AccelerationProfile", (params.get_int("AccelerationProfile") + 1) % 4)
#         mem_params.put_bool("FrogPilotTogglesUpdated", True)

#     elif code == 'L':  # 車距設定
#         params.put_bool("Speeddistance", False)
#         params.put_int("LongitudinalPersonality", (params.get_int("LongitudinalPersonality") + 2) % 3)
#         mem_params.put_bool("FrogPilotTogglesUpdated", True)

#     # 速度微調（旋鈕）
#     elif code == '0':  # 恢復上次速度
#         if mem_params.get_int("SpeedPrev") != 0:
#             newsetspeed = mem_params.get_int("SpeedPrev")
#             mem_params.put_int("SpeedPrev", 0)
#         mem_params.put_bool("KeyResume", True)

#     elif code in ['1', '2', '6', '7']:  # 速度增減
#         speed_steps = {'1': -1, '2': +1, '6': -10, '7': +10}
#         step = speed_steps.get(code, 0)

#         if road_type == 2:
#             step = step if step < 0 else step * 5
#         elif road_type == 3:
#             step = step if step < 0 else step * 10

#         newsetspeed = max(0, min(140, setspeed + step))

#         # 確保速度符合 5 或 10 的倍數
#         if road_type == 2:
#             newsetspeed = round(newsetspeed / 5) * 5
#         elif road_type == 3:
#             newsetspeed = round(newsetspeed / 10) * 10

#         mem_params.put_bool("KeyResume", True)

#     # elif code == '3':


#     elif code == '5':
#         mem_params.put_bool("KeyCancel", True)# 取消速度設定
#         params.put_bool("Speeddistance", True)# 速度與距離模式開啟
#         mem_params.put_bool("FrogPilotTogglesUpdated", True)
#     return newsetspeed


def selekey(code,setspeed):
    newsetspeed=setspeed
    newRoadtypeProfile=params.get_int("RoadtypeProfile")
#按鍵
    #上__速限控制
    if code == 'A':
        if params.get_int("RoadtypeProfile") == 0:
            newsetspeed =  40
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 1:
            newsetspeed =  20
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 2:
            newsetspeed =  50
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 3:
            newsetspeed =  60
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
    elif code == 'B':
        if params.get_int("RoadtypeProfile") == 0:
            newsetspeed =  50
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 1:
            newsetspeed =  50
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 2:
            newsetspeed =  80
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 3:
            newsetspeed =  100
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
    elif code == 'C':
        if params.get_int("RoadtypeProfile") == 0:
            newsetspeed =  60
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 1:
            newsetspeed =  60
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 2:
            newsetspeed =  90
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 3:
            newsetspeed =  110
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
    elif code == 'D':
        if params.get_int("RoadtypeProfile") == 0:
            newsetspeed =  70
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 1:
            newsetspeed =  70
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 2:
            newsetspeed =  100
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
        elif params.get_int("RoadtypeProfile") == 3:
            newsetspeed =  120
            mem_params.put_int('SpeedPrev',0)
            mem_params.put_bool("KeyResume", True)
    #中__導航控制
    elif code == 'E':
        if mem_params.get_int("DetectSpeedLimit") != 0:
            mem_params.put_bool('SpeedLimitChanged', True)
        if params.get_bool("TrafficMode"):
            params.put_bool("TrafficMode" , False)
            # mem_params.put_bool("TrafficModeActive", False)
        else:
            params.put_bool("TrafficMode" , True)
            # mem_params.put_bool("TrafficModeActive", True)
    elif code == 'F':
            params.remove("NavDestination")
    elif code == 'G':
        params.put("NavDestination", "{\"latitude\": %f, \"longitude\": %f, \"place_name\": \"%s\"}" % (24.2103369, 120.5764539, "\u6b63\u82f1\u4e03\u8857"))
    elif code == 'H':
        params.put("NavDestination", "{\"latitude\": %f, \"longitude\": %f, \"place_name\": \"%s\"}" % (24.184996, 120.603791, "\u53f0\u4e2d\u69ae\u7e3d"))

    #下__設定
    elif code == 'I':#切換自動ACC或手動啟動ACC
        # params.put_bool("AutoACC", True)
        autoaccProfile = params.get_bool("AutoACC")
        autoaccProfile = not autoaccProfile
        params.put_bool("AutoACC", autoaccProfile)
        mem_params.put_bool("FrogPilotTogglesUpdated", True)
        # params.put_bool("KeyResume", True)
    elif code == 'J':#道路選擇
        newRoadtypeProfile = params.get_int("RoadtypeProfile") + 1
        if newRoadtypeProfile > 5:
            newRoadtypeProfile = 0  # 超過 5 則重設為 0
        params.put_int("RoadtypeProfile", newRoadtypeProfile)
        # 只有當 newRoadtypeProfile == 5 時 AutoRoadtype 才是 True，否則為 False
        params.put_bool("AutoRoadtype", newRoadtypeProfile == 5)
        mem_params.put_bool("FrogPilotTogglesUpdated", True)
    elif code == 'K':#加速選擇
        accelerationProfile = params.get_int("AccelerationProfile") + 1
        if accelerationProfile > 3:
            accelerationProfile = 0
        params.put_int("AccelerationProfile", accelerationProfile)
        mem_params.put_bool("FrogPilotTogglesUpdated", True)
    elif code == 'L':#車距選擇
        params.put_bool("Speeddistance", False)
        personalityProfile = (params.get_int("LongitudinalPersonality") + 2) % 3
        params.put_int("LongitudinalPersonality", personalityProfile)
        mem_params.put_bool("FrogPilotTogglesUpdated", True)

    #左旋鈕
    elif code == '0' :
        mem_params.put_bool("KeyResume", True)
        if mem_params.get_int("SpeedPrev") != 0:
            newsetspeed = mem_params.get_int("SpeedPrev")
            mem_params.put_int("SpeedPrev",0)
    elif code == '1':#左旋-5
        if params.get_int("RoadtypeProfile") == 1:
            mem_params.put_bool("KeyResume", True)
            if setspeed - 1 < 0:
                newsetspeed = 0
            else:
                newsetspeed = setspeed - 1
        elif params.get_int("RoadtypeProfile") == 2:
            mem_params.put_bool("KeyResume", True)
            if setspeed - 5 < 0:
                newsetspeed = 0
            else:
                newsetspeed = setspeed - 5
                if newsetspeed % 5 != 0 :
                    newsetspeed=math.floor(newsetspeed / 5) * 5
        elif params.get_int("RoadtypeProfile") == 3:
            mem_params.put_bool("KeyResume", True)
            if setspeed - 10 < 0:
                newsetspeed = 0
            else:
                newsetspeed = setspeed - 10
                if newsetspeed % 10 != 0 :
                    newsetspeed=math.floor(newsetspeed / 10) * 10
    elif code == '2':#右璇+5
        if params.get_int("RoadtypeProfile") == 1:
            mem_params.put_bool("KeyResume", True)
            if setspeed + 1 > 140:
                newsetspeed = 140
            else:
                newsetspeed = setspeed + 1
        elif params.get_int("RoadtypeProfile") == 2:
            mem_params.put_bool("KeyResume", True)
            if setspeed + 5 > 140:
                newsetspeed = 140
            else:
                newsetspeed = setspeed + 5
                if newsetspeed % 5 != 0 :
                    newsetspeed=math.ceil(newsetspeed / 5) * 5
        elif params.get_int("RoadtypeProfile") == 3:
            mem_params.put_bool("KeyResume", True)
            if setspeed + 10 > 140:
                newsetspeed = 140
            else:
                newsetspeed = setspeed + 10
                if newsetspeed % 10 != 0 :
                    newsetspeed=math.ceil(newsetspeed / 10) * 10
    elif code == '3':#壓左旋
        params.put_bool("Speeddistance", True)
        mem_params.put_bool("FrogPilotTogglesUpdated", True)
        # mem_params.put_int('DetectSpeedLimit', 0 )
        # elif code == '4':#壓右璇+1

    #右旋鈕
    elif code == '5' :
        mem_params.put_bool("KeyCancel", True)
    elif code == '6':#左旋-10
        if params.get_int("RoadtypeProfile") == 1:
            if setspeed - 5 < 0:
                newsetspeed = 0
            else:
                newsetspeed = setspeed - 5
                if newsetspeed % 5 != 0 :
                    newsetspeed=math.floor(newsetspeed / 5) * 5
        elif params.get_int("RoadtypeProfile") == 2:
            if setspeed - 10 < 0:
                newsetspeed = 0
            else:
                newsetspeed = setspeed - 10
                if newsetspeed % 10 != 0 :
                    newsetspeed=math.floor(newsetspeed / 10) * 10
        elif params.get_int("RoadtypeProfile") == 3:
            if setspeed - 20 < 0:
                newsetspeed = 0
            else:
                newsetspeed = setspeed - 20
                if newsetspeed % 20 != 0 :
                    newsetspeed=math.floor(newsetspeed / 20) * 20
    elif code == '7':#右璇+10
        if params.get_int("RoadtypeProfile") == 1:
            if setspeed + 5 < 0:
                newsetspeed = 0
            else:
                newsetspeed = setspeed + 5
                if newsetspeed % 5 != 0 :
                    newsetspeed=math.floor(newsetspeed / 5) * 5
        elif params.get_int("RoadtypeProfile") == 2:
            if setspeed + 10 < 0:
                newsetspeed = 0
            else:
                newsetspeed = setspeed + 10
                if newsetspeed % 10 != 0 :
                    newsetspeed=math.floor(newsetspeed / 10) * 10
        elif params.get_int("RoadtypeProfile") == 3:
            if setspeed + 20 < 0:
                newsetspeed = 0
            else:
                newsetspeed = setspeed + 20
                if newsetspeed % 20 != 0 :
                    newsetspeed=math.floor(newsetspeed / 20) * 20

    return newsetspeed

def main():
    device = evdev.InputDevice('/dev/input/event3')
    for event in device.read_loop():
        set_speed = mem_params.get_int('KeySetSpeed')
        key_setspeed = set_speed
        data = evdev.categorize(event)
        if event.type == evdev.ecodes.EV_KEY and data.keystate == 0:
            keyin = evdev.ecodes.KEY[event.code][4:]
            key_setspeed = selekey(keyin,set_speed)
        new_setspeed = key_setspeed
        if new_setspeed != set_speed:
            mem_params.put_int('KeySetSpeed', new_setspeed)
            mem_params.put_bool('KeyChanged', True)
            mem_params.put_bool('SpeedLimitChanged', False)

if __name__ == "__main__":
    main()

