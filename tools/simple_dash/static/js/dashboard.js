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
      mutcdValue: document.getElementById('mutcd-value'),

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

      // 加減速度
      accelerationValue: document.getElementById('acceleration-value'),

      // 道路名稱
      roadNameCard: document.getElementById('road-name-card'),
      roadNameText: document.getElementById('road-name-text'),

      // 狀態指示器
      statusText: document.getElementById('status-text'),
      statusDot: document.querySelector('.status-dot')
    };

    // 狀態
    this.state = {
      isMetric: true,
      hasLead: false,
      vEgo: 0,
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
    // 使用 MAPD 原始資料 (與 FrogPilot UI 一致)
    // 從 params_memory.MapSpeedLimit 讀取，未經 SLC 處理
    const speedLimit = frogpilotPlan.mapdSpeedLimit || 0;  // m/s

    // 永遠顯示速限，沒有數據時顯示 --
    if (speedLimit <= 0) {
      this.elements.mutcdValue.textContent = '--';
    } else {
      // 直接轉換單位 (m/s -> km/h 或 mph)
      const speedLimitConverted = speedLimit * this.units.speedConversion;
      this.elements.mutcdValue.textContent = Math.round(speedLimitConverted).toString();
    }

    // 即將到來的速限 - 永遠顯示，數值和距離分別處理
    const upcomingLimit = frogpilotPlan.slcNextSpeedLimit || 0;
    const upcomingDistance = frogpilotPlan.slcNextSpeedLimitDistance || 0;

    // 速限數值
    if (upcomingLimit > 0) {
      const upcomingConverted = upcomingLimit * this.units.speedConversion;
      this.elements.upcomingValue.textContent = Math.round(upcomingConverted);
    } else {
      this.elements.upcomingValue.textContent = '--';
    }

    // 距離
    if (upcomingDistance > 0) {
      const distanceConverted = upcomingDistance * this.units.distanceConversion;
      this.elements.upcomingDistance.textContent = `${Math.round(distanceConverted)} ${this.units.distance}`;
    } else {
      this.elements.upcomingDistance.textContent = `-- ${this.units.distance}`;
    }
  }

  // ========== CSC 彎道速度控制更新 ==========

  updateCSC(frogpilotPlan) {
    const cscControlling = frogpilotPlan.cscControllingSpeed || false;
    const cscTraining = frogpilotPlan.cscTraining || false;
    const cscSpeed = frogpilotPlan.cscSpeed || 0;
    const roadCurvature = frogpilotPlan.roadCurvature || 0;

    // 永遠顯示 CSC 卡片

    // 彎道方向 (負值 = 左彎，正值 = 右彎)
    if (roadCurvature < 0) {
      this.elements.cscIcon.classList.remove('flip');
    } else {
      this.elements.cscIcon.classList.add('flip');
    }

    // 速度顯示 - 沒有數據或未控制時顯示 --
    if (cscSpeed > 0 && (cscControlling || cscTraining)) {
      const cscSpeedConverted = cscSpeed * this.units.speedConversion;
      this.elements.cscSpeed.textContent = Math.round(cscSpeedConverted);
    } else {
      this.elements.cscSpeed.textContent = '--';
    }

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

  updateCEM(frogpilotPlan) {
    const conditionalStatus = frogpilotPlan.ceStatus || 0;
    const experimentalMode = frogpilotPlan.experimentalMode || false;

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

  // ========== 加減速度更新 ==========

  updateAcceleration(aEgo) {
    if (aEgo === undefined || aEgo === null) {
      this.elements.accelerationValue.textContent = '--';
      this.elements.accelerationValue.className = 'value-medium neutral';
      return;
    }

    // 顯示加速度數值 (保留 2 位小數)
    this.elements.accelerationValue.textContent = aEgo.toFixed(2);

    // 移除所有顏色類別
    this.elements.accelerationValue.classList.remove('positive', 'negative', 'neutral');

    // 根據加速度值設定顏色
    if (aEgo > 0.25) {
      // 加速 (綠色)
      this.elements.accelerationValue.classList.add('positive');
    } else if (aEgo < -0.25) {
      // 煞車 (紅色)
      this.elements.accelerationValue.classList.add('negative');
    } else {
      // 中性 (灰色)
      this.elements.accelerationValue.classList.add('neutral');
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
