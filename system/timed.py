#!/usr/bin/env python3
import datetime
import os
import signal
import subprocess
import sys
import time
from typing import NoReturn

from timezonefinder import TimezoneFinder

import cereal.messaging as messaging
from openpilot.common.time import system_time_valid
from openpilot.common.params import Params
from openpilot.common.swaglog import cloudlog
from openpilot.system.hardware import AGNOS


def set_timezone(timezone):
  valid_timezones = subprocess.check_output('timedatectl list-timezones', shell=True, encoding='utf8').strip().split('\n')
  if timezone not in valid_timezones:
    cloudlog.error(f"Timezone not supported {timezone}")
    return

  cloudlog.debug(f"Setting timezone to {timezone}")
  try:
    if AGNOS:
      tzpath = os.path.join("/usr/share/zoneinfo/", timezone)
      subprocess.check_call(f'sudo su -c "ln -snf {tzpath} /data/etc/tmptime && \
                              mv /data/etc/tmptime /data/etc/localtime"', shell=True)
      subprocess.check_call(f'sudo su -c "echo \"{timezone}\" > /data/etc/timezone"', shell=True)
    else:
      subprocess.check_call(f'sudo timedatectl set-timezone {timezone}', shell=True)
  except subprocess.CalledProcessError:
    cloudlog.exception(f"Error setting timezone to {timezone}")


def set_time(new_time):
  diff = datetime.datetime.now() - new_time
  if diff < datetime.timedelta(seconds=10):
    cloudlog.debug(f"Time diff too small: {diff}")
    return

  cloudlog.debug(f"Setting time to {new_time}")
  try:
    subprocess.run(f"TZ=UTC date -s '{new_time}'", shell=True, check=True)
  except subprocess.CalledProcessError:
    cloudlog.exception("timed.failed_setting_time")


def save_time_to_param(params: Params):
  """保存當前時間到 PARAMS（關機時調用）"""
  if system_time_valid():
    current_timestamp = time.time()
    params.put("LastKnownGoodTime", str(current_timestamp))
    cloudlog.info(f"Saved LastKnownGoodTime: {datetime.datetime.fromtimestamp(current_timestamp)}")


def signal_handler(signum, frame):
  """處理關機信號，保存時間後退出"""
  cloudlog.info(f"Received signal {signum}, saving time before exit")
  params = Params()
  save_time_to_param(params)
  sys.exit(0)


def main() -> NoReturn:
  """
    timed has three responsibilities:
    - getting the current time
    - getting the current timezone
    - persisting last known good time for recovery

    GPS directly gives time, and timezone is looked up from GPS position.
    AGNOS will also use NTP to update the time.
    On shutdown, saves current time to recover on next boot if GPS unavailable.
  """

  params = Params()

  # 註冊關機信號處理器
  signal.signal(signal.SIGTERM, signal_handler)
  signal.signal(signal.SIGINT, signal_handler)

  # Restore timezone from param
  tz = params.get("Timezone", encoding='utf8')
  tf = TimezoneFinder()
  if tz is not None:
    cloudlog.debug("Restoring timezone from param")
    set_timezone(tz)

  # 如果系統時間無效，嘗試從 PARAMS 恢復
  if not system_time_valid():
    last_good_timestamp_str = params.get("LastKnownGoodTime", encoding='utf8')
    if last_good_timestamp_str is not None:
      try:
        last_good_timestamp = float(last_good_timestamp_str)
        last_good_time = datetime.datetime.fromtimestamp(last_good_timestamp)
        cloudlog.info(f"Restoring system time from LastKnownGoodTime: {last_good_time}")
        set_time(last_good_time)
      except (ValueError, TypeError) as e:
        cloudlog.error(f"Failed to parse LastKnownGoodTime: {e}")

  pm = messaging.PubMaster(['clocks'])
  sm = messaging.SubMaster(['liveLocationKalman'])
  while True:
    sm.update(1000)

    msg = messaging.new_message('clocks')
    msg.valid = system_time_valid()
    msg.clocks.wallTimeNanos = time.time_ns()
    pm.send('clocks', msg)

    llk = sm['liveLocationKalman']
    if not llk.gpsOK or (time.monotonic() - sm.logMonoTime['liveLocationKalman']/1e9) > 0.2:
      continue

    # set time
    # TODO: account for unixTimesatmpMillis being a (usually short) time in the past
    gps_time = datetime.datetime.fromtimestamp(llk.unixTimestampMillis / 1000.)
    set_time(gps_time)

    # GPS 同步後立即保存時間（這是唯一的寫入時機）
    params.put_nonblocking("LastKnownGoodTime", str(llk.unixTimestampMillis / 1000.))
    cloudlog.debug(f"Saved time from GPS: {gps_time.isoformat()}")

    # set timezone
    pos = llk.positionGeodetic.value
    if len(pos) == 3:
      gps_timezone = tf.timezone_at(lat=pos[0], lng=pos[1])
      if gps_timezone is None:
        cloudlog.critical(f"No timezone found based on {pos=}")
      else:
        set_timezone(gps_timezone)
        params.put_nonblocking("Timezone", gps_timezone)

    time.sleep(10)

if __name__ == "__main__":
  main()
