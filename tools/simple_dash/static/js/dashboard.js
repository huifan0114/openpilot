/**
 * Dashboard UI - 核心邏輯
 * 處理所有儀表板元素的更新和狀態管理
 */

export class DashboardUI {
  constructor() {
    // DOM 元素引用
    this.elements = {
      // 前車資訊
      leadCard: document.getElementById('lead-card'),
      noLeadState: document.getElementById('no-lead-state'),
      hasLeadState: document.getElementById('has-lead-state'),
      leadDistance: document.getElementById('lead-distance'),
      desiredDistance: document.getElementById('desired-distance'),
      leadSpeed: document.getElementById('lead-speed'),
      followTime: document.getElementById('follow-time'),

      // 當前速度
      currentSpeed: document.getElementById('current-speed-value'),
      currentSpeedUnit: document.getElementById('current-speed-unit'),

      // MAX 速度
      maxSpeedCard: document.getElementById('max-speed-card'),
      maxSpeed: document.getElementById('max-speed-value'),
      maxSpeedUnit: document.getElementById('max-speed-unit'),

      // 速限標誌
      speedLimitCard: document.getElementById('speed-limit-card'),
      mutcdSign: document.getElementById('mutcd-sign'),
      mutcdValue: document.getElementById('mutcd-value'),
      viennaSign: document.getElementById('vienna-sign'),
      viennaValue: document.getElementById('vienna-value'),

      // 即將到來的速限
      upcomingLimitCard: document.getElementById('upcoming-limit-card'),
      upcomingValue: document.getElementById('upcoming-value'),
      upcomingDistance: document.getElementById('upcoming-distance'),

      // CSC 彎道速度控制
      cscCard: document.getElementById('csc-card'),
      cscIcon: document.getElementById('csc-icon'),
      cscSpeed: document.getElementById('csc-speed'),
      cscSpeedUnit: document.getElementById('csc-speed-unit'),
      cscTraining: document.getElementById('csc-training'),

      // CEM 狀態
      cemCard: document.getElementById('cem-card'),
      cemIcon: document.getElementById('cem-icon'),

      // 踏板
      brakePedal: document.getElementById('brake-pedal'),
      gasPedal: document.getElementById('gas-pedal'),

      // 道路名稱
      roadNameCard: document.getElementById('road-name-card'),
      roadNameText: document.getElementById('road-name-text'),

      // 停車計時器
      standstillCard: document.getElementById('standstill-timer-card'),
      standstillMinutes: document.getElementById('standstill-minutes'),
      standstillSeconds: document.getElementById('standstill-seconds'),

      // 狀態指示器
      statusText: document.getElementById('status-text'),
      statusDot: document.querySelector('.status-dot')
    };

    // 狀態
    this.state = {
      isMetric: true,
      hasLead: false,
      vEgo: 0,
      isViennaSign: false,
      cachedRadarState: null,
      cachedFrogpilotPlan: null
    };

    // 單位
    this.units = {
      distance: '米',
      speed: 'km/h',
      distanceConversion: 1.0,
      speedConversion: 3.6  // m/s to km/h
    };

    console.log('[Dashboard] Initialized');
  }

  // ========== 前車資訊更新 (核心功能) ==========

  updateLeadInfo(radarState, frogpilotPlan) {
    // 快取狀態供其他函數使用
    this.state.cachedRadarState = radarState;
    this.state.cachedFrogpilotPlan = frogpilotPlan;

    if (!radarState || !radarState.leadOne) {
      this.showNoLead();
      return;
    }

    const lead = radarState.leadOne;

    // 檢查前車是否有效 (dRel > 0 代表有前車)
    if (!lead.dRel || lead.dRel <= 0) {
      this.showNoLead();
      return;
    }

    this.state.hasLead = true;
    this.elements.noLeadState.classList.add('hidden');
    this.elements.hasLeadState.classList.remove('hidden');

    // 計算距離
    const distance = lead.dRel * this.units.distanceConversion;
    const desiredDist = (frogpilotPlan?.desiredFollowDistance || 0) * this.units.distanceConversion;

    // 計算速度 (確保非負)
    const leadSpeed = Math.max(0, lead.vLead || 0) * this.units.speedConversion;

    // 計算跟車時間
    const followTime = this.state.vEgo > 1
      ? (lead.dRel / this.state.vEgo).toFixed(1)
      : '--';

    // 更新顯示
    this.elements.leadDistance.textContent = `${Math.round(distance)} ${this.units.distance}`;
    this.elements.desiredDistance.textContent = `(期望: ${Math.round(desiredDist)} ${this.units.distance})`;
    this.elements.leadSpeed.textContent = `${Math.round(leadSpeed)} ${this.units.speed}`;
    this.elements.followTime.textContent = followTime !== '--' ? `${followTime} 秒` : '--';

    // 根據距離和時間更新卡片狀態
    this.updateLeadCardStatus(distance, desiredDist, followTime);
  }

  showNoLead() {
    this.state.hasLead = false;
    this.elements.hasLeadState.classList.add('hidden');
    this.elements.noLeadState.classList.remove('hidden');

    // 移除所有狀態類
    this.elements.leadCard.classList.remove('status-safe', 'status-warning', 'status-danger');
  }

  updateLeadCardStatus(distance, desiredDist, followTime) {
    const card = this.elements.leadCard;

    // 移除所有狀態類
    card.classList.remove('status-safe', 'status-warning', 'status-danger');
    this.elements.leadDistance.classList.remove('status-safe', 'status-warning', 'status-danger');
    this.elements.followTime.classList.remove('status-safe', 'status-warning', 'status-danger');

    // 優先根據跟車時間判斷 (更重要)
    if (followTime !== '--') {
      const time = parseFloat(followTime);
      if (time < 1.5) {
        card.classList.add('status-danger');
        this.elements.followTime.classList.add('status-danger');
        return;
      } else if (time < 2.5) {
        card.classList.add('status-warning');
        this.elements.followTime.classList.add('status-warning');
        return;
      }
    }

    // 根據距離判斷
    const diff = distance - desiredDist;
    if (diff < -5) {
      card.classList.add('status-danger');
      this.elements.leadDistance.classList.add('status-danger');
    } else if (diff < 5) {
      card.classList.add('status-warning');
      this.elements.leadDistance.classList.add('status-warning');
    } else {
      card.classList.add('status-safe');
      this.elements.leadDistance.classList.add('status-safe');
    }
  }

  // ========== 當前速度更新 ==========

  updateCurrentSpeed(vEgo) {
    this.state.vEgo = vEgo;
    const speed = vEgo * this.units.speedConversion;
    this.elements.currentSpeed.textContent = Math.round(speed);

    // 如果有快取的雷達資料，重新計算跟車時間
    if (this.state.cachedRadarState) {
      this.updateLeadInfo(this.state.cachedRadarState, this.state.cachedFrogpilotPlan);
    }
  }

  // ========== MAX 速度更新 ==========

  updateMaxSpeed(vCruise) {
    if (!vCruise || vCruise === 0 || vCruise === 255) {
      this.elements.maxSpeed.textContent = '--';
      return;
    }

    const maxSpeed = vCruise * this.units.speedConversion;
    this.elements.maxSpeed.textContent = Math.round(maxSpeed);
  }

  // ========== 速限標誌更新 ==========

  updateSpeedLimit(frogpilotPlan) {
    const speedLimit = frogpilotPlan.slcSpeedLimit || 0;

    if (speedLimit <= 0) {
      this.elements.speedLimitCard.classList.add('hidden');
      return;
    }

    this.elements.speedLimitCard.classList.remove('hidden');

    const speedLimitConverted = speedLimit * this.units.speedConversion;
    const speedLimitStr = Math.round(speedLimitConverted).toString();

    if (this.state.isViennaSign) {
      // Vienna 歐式圓形標誌
      this.elements.mutcdSign.classList.add('hidden');
      this.elements.viennaSign.classList.remove('hidden');
      this.elements.viennaValue.textContent = speedLimitStr;
    } else {
      // MUTCD 美式方形標誌
      this.elements.viennaSign.classList.add('hidden');
      this.elements.mutcdSign.classList.remove('hidden');
      this.elements.mutcdValue.textContent = speedLimitStr;
    }

    // 即將到來的速限
    const upcomingLimit = frogpilotPlan.slcNextSpeedLimit || 0;
    const upcomingDistance = frogpilotPlan.slcNextSpeedLimitDistance || 0;

    if (upcomingLimit > 0 && upcomingDistance > 0) {
      this.elements.upcomingLimitCard.classList.remove('hidden');
      const upcomingConverted = upcomingLimit * this.units.speedConversion;
      const distanceConverted = upcomingDistance * this.units.distanceConversion;
      this.elements.upcomingValue.textContent = Math.round(upcomingConverted);
      this.elements.upcomingDistance.textContent = `${Math.round(distanceConverted)} ${this.units.distance}`;
    } else {
      this.elements.upcomingLimitCard.classList.add('hidden');
    }
  }

  // ========== CSC 彎道速度控制更新 ==========

  updateCSC(frogpilotPlan) {
    const cscControlling = frogpilotPlan.cscControllingSpeed || false;
    const cscTraining = frogpilotPlan.cscTraining || false;
    const cscSpeed = frogpilotPlan.cscSpeed || 0;
    const roadCurvature = frogpilotPlan.roadCurvature || 0;

    if (!cscControlling && !cscTraining) {
      this.elements.cscCard.classList.add('hidden');
      return;
    }

    this.elements.cscCard.classList.remove('hidden');

    // 彎道方向 (負值 = 左彎，正值 = 右彎)
    if (roadCurvature < 0) {
      this.elements.cscIcon.classList.remove('flip');
    } else {
      this.elements.cscIcon.classList.add('flip');
    }

    // 速度顯示
    const cscSpeedConverted = cscSpeed * this.units.speedConversion;
    this.elements.cscSpeed.textContent = Math.round(cscSpeedConverted);

    // 訓練模式
    if (cscTraining) {
      this.elements.cscCard.classList.add('training');
      this.elements.cscTraining.classList.remove('hidden');
    } else {
      this.elements.cscCard.classList.remove('training');
      this.elements.cscTraining.classList.add('hidden');
    }
  }

  // ========== CEM 狀態更新 ==========

  updateCEM(frogpilotPlan, controlsState) {
    const conditionalStatus = frogpilotPlan.ceStatus || 0;
    const experimentalMode = controlsState?.experimentalMode || false;

    if (!experimentalMode && conditionalStatus === 0) {
      this.elements.cemCard.classList.add('hidden');
      return;
    }

    this.elements.cemCard.classList.remove('hidden');

    // 根據狀態選擇圖標 (使用 emoji 作為臨時圖標)
    let icon = '🤖';  // 預設
    if (conditionalStatus === 1) {
      icon = '😌';  // Chill Mode (Overridden)
    } else if (conditionalStatus === 2 || experimentalMode) {
      icon = '🚀';  // Experimental Mode
    } else if (conditionalStatus === 3 || conditionalStatus === 4) {
      icon = '⚡';  // Speed-based
    } else if (conditionalStatus === 5 || conditionalStatus === 7) {
      icon = '↪️';  // Turn-based
    } else if (conditionalStatus === 6 || conditionalStatus === 11 || conditionalStatus === 12) {
      icon = '🚦';  // Stop Light
    } else if (conditionalStatus === 8) {
      icon = '🌀';  // Curve
    } else if (conditionalStatus === 9 || conditionalStatus === 10) {
      icon = '🚗';  // Lead
    }

    this.elements.cemIcon.textContent = icon;
  }

  // ========== 踏板更新 ==========

  updatePedals(aEgo, brakeLights) {
    // 煞車踏板 (加速度 < -0.25 或煞車燈亮起)
    if (aEgo < -0.25 || brakeLights) {
      this.elements.brakePedal.classList.add('active');
      // 動態透明度
      const brakeOpacity = Math.min(1.0, Math.abs(aEgo) * 2);
      this.elements.brakePedal.style.opacity = Math.max(0.3, brakeOpacity);
    } else {
      this.elements.brakePedal.classList.remove('active');
      this.elements.brakePedal.style.opacity = 0.3;
    }

    // 油門踏板 (加速度 > 0.25)
    if (aEgo > 0.25) {
      this.elements.gasPedal.classList.add('active');
      // 動態透明度
      const gasOpacity = Math.min(1.0, aEgo * 2);
      this.elements.gasPedal.style.opacity = Math.max(0.3, gasOpacity);
    } else {
      this.elements.gasPedal.classList.remove('active');
      this.elements.gasPedal.style.opacity = 0.3;
    }
  }

  // ========== 道路名稱更新 ==========

  updateRoadName(roadName) {
    if (!roadName || roadName.trim() === '') {
      this.elements.roadNameCard.classList.add('hidden');
      return;
    }

    this.elements.roadNameCard.classList.remove('hidden');
    this.elements.roadNameText.textContent = roadName;
  }

  // ========== 停車計時器更新 ==========

  updateStandstillTimer(duration) {
    if (!duration || duration === 0) {
      this.elements.standstillCard.classList.add('hidden');
      return;
    }

    this.elements.standstillCard.classList.remove('hidden');

    const minutes = Math.floor(duration / 60);
    const seconds = duration % 60;

    this.elements.standstillMinutes.textContent = `${minutes} 分鐘`;
    this.elements.standstillSeconds.textContent = `${seconds} 秒`;
  }

  // ========== 連接狀態更新 ==========

  updateConnectionStatus(connected, text) {
    if (this.elements.statusText) {
      this.elements.statusText.textContent = text || (connected ? '已連接' : '連接中...');
    }

    if (this.elements.statusDot) {
      if (connected) {
        this.elements.statusDot.classList.add('connected');
      } else {
        this.elements.statusDot.classList.remove('connected');
      }
    }
  }

  // ========== 單位切換 ==========

  setMetric(isMetric) {
    this.state.isMetric = isMetric;

    if (isMetric) {
      this.units.distance = '米';
      this.units.speed = 'km/h';
      this.units.distanceConversion = 1.0;
      this.units.speedConversion = 3.6;  // m/s to km/h
    } else {
      this.units.distance = '英尺';
      this.units.speed = 'mph';
      this.units.distanceConversion = 3.28084;  // meters to feet
      this.units.speedConversion = 2.23694;  // m/s to mph
    }

    // 更新單位顯示
    this.elements.currentSpeedUnit.textContent = this.units.speed;
    this.elements.maxSpeedUnit.textContent = this.units.speed;
    this.elements.cscSpeedUnit.textContent = this.units.speed;
  }

  // ========== 速限標誌樣式切換 ==========

  setSpeedLimitStyle(isVienna) {
    this.state.isViennaSign = isVienna;
  }
}
