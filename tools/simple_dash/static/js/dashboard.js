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

      // MAX 速度
      maxSpeedCard: document.getElementById('max-speed-card'),
      maxSpeed: document.getElementById('max-speed-value'),

      // VCRUISE
      vcruiseCard: document.getElementById('vcruise-card'),
      vcruiseValue: document.getElementById('vcruise-value'),

      // 速限標誌
      speedLimitCard: document.getElementById('speed-limit-card'),
      speedLimitValue: document.getElementById('speed-limit-value'),

      // 即將到來的速限
      upcomingLimitCard: document.getElementById('upcoming-limit-card'),
      upcomingValue: document.getElementById('upcoming-value'),
      upcomingDistance: document.getElementById('upcoming-distance'),

      // CSC 彎道速度控制
      cscCard: document.getElementById('csc-card'),
      cscSpeed: document.getElementById('csc-speed'),

      // EXPERIMENTAL MODE
      experimentalCard: document.getElementById('experimental-card'),
      experimentalIcon: document.getElementById('experimental-icon'),

      // 加減速度
      accelerationValue: document.getElementById('acceleration-value'),

      // 道路名稱
      roadNameCard: document.getElementById('road-name-card'),
      roadNameText: document.getElementById('road-name-text'),

      // 連接狀態
      connectionStatus: document.getElementById('connection-status'),
      statusText: document.getElementById('status-text'),

      // 盲點警示
      leftBlindspot: document.getElementById('left-blindspot'),
      rightBlindspot: document.getElementById('right-blindspot'),

      // CEM 狀態
      cemCard: document.getElementById('cem-card'),
      cemIcon: document.getElementById('cem-icon'),
      cemText: document.getElementById('cem-text')
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

  // ========== 飛機儀表板啟動效果 ==========

  /**
   * 啟動儀表板自檢效果
   * 模擬飛機儀表板開機時的完整 LCD 測試
   * 包含所有元素：數值、盲點、EXPERIMENTAL MODE 等
   */
  startupSequence() {
    console.log('[Dashboard] 🎬 Starting startup sequence...');

    // 添加啟動模式類
    document.body.classList.add('startup-mode');

    // 測試數值模板
    const testValues = {
      large: '888',    // 3位數
      medium: '88',    // 2位數
      distance: '888', // 距離
      time: '8.8'      // 時間
    };

    // === 1. 所有數值顯示測試模式 ===

    // MAX 和 VCRUISE
    this.elements.maxSpeed.textContent = testValues.large;
    this.elements.vcruiseValue.textContent = testValues.large;

    // 速限
    this.elements.speedLimitValue.textContent = testValues.medium;
    this.elements.upcomingValue.textContent = testValues.medium;
    this.elements.upcomingDistance.textContent = `${testValues.distance} ${this.units.distance}`;

    // CSC
    this.elements.cscSpeed.textContent = testValues.medium;

    // 加減速度
    this.elements.accelerationValue.textContent = '8.88';
    this.elements.accelerationValue.classList.add('positive');

    // 前車資訊（強制顯示，移除單位）
    this.elements.noLeadState.classList.add('hidden');
    this.elements.hasLeadState.classList.remove('hidden');
    this.elements.leadDistance.textContent = testValues.distance;
    this.elements.desiredDistance.textContent = testValues.distance;
    this.elements.leadSpeed.textContent = testValues.medium;
    this.elements.followTime.textContent = testValues.time;

    // 道路名稱
    this.elements.roadNameCard.classList.remove('hidden');
    this.elements.roadNameText.textContent = '888888888';

    // === 2. 盲點警示顯示 ===
    this.elements.leftBlindspot.classList.remove('hidden');
    this.elements.leftBlindspot.classList.add('active');
    this.elements.rightBlindspot.classList.remove('hidden');
    this.elements.rightBlindspot.classList.add('active');

    // === 3. EXPERIMENTAL MODE 圖標顯示 ===
    // ⚠️ 暫時註解
    // this.elements.experimentalIcon.src = '/static/assets/img_experimental.svg';
    // this.elements.experimentalCard.classList.add('active');

    // === 4. 定義完整淡出序列（按邏輯順序熄滅）===
    const fadeoutSequence = [
      // 頂部元素
      { element: this.elements.maxSpeed, delay: 800, type: 'value' },
      { element: this.elements.vcruiseValue, delay: 1000, type: 'value' },
      { element: this.elements.speedLimitValue, delay: 1200, type: 'value' },
      { element: this.elements.upcomingValue, delay: 1400, type: 'value' },

      // 盲點警示
      { element: this.elements.leftBlindspot, delay: 1600, type: 'blindspot' },
      { element: this.elements.rightBlindspot, delay: 1800, type: 'blindspot' },

      // EXPERIMENTAL MODE
      // ⚠️ 暫時註解
      // { element: this.elements.experimentalCard, delay: 2000, type: 'experimental' },

      // 前車資訊
      { element: this.elements.leadDistance, delay: 2200, type: 'value' },
      { element: this.elements.leadSpeed, delay: 2400, type: 'value' },
      { element: this.elements.followTime, delay: 2600, type: 'value' },

      // 底部元素
      { element: this.elements.cscSpeed, delay: 2800, type: 'value' },
      { element: this.elements.accelerationValue, delay: 3000, type: 'value' },
      { element: this.elements.roadNameCard, delay: 3200, type: 'card' }
    ];

    // === 5. 執行淡出序列 ===
    fadeoutSequence.forEach(({ element, delay, type }) => {
      setTimeout(() => {
        if (type === 'value') {
          // 數值淡出
          element.classList.add('startup-fadeout');
          setTimeout(() => {
            element.textContent = '--';
            element.classList.remove('startup-fadeout');
          }, 1600);

        } else if (type === 'blindspot') {
          // 盲點淡出並隱藏
          element.classList.add('blindspot-fadeout');
          setTimeout(() => {
            element.classList.remove('active', 'blindspot-fadeout');
            element.classList.add('hidden');
          }, 1600);

        } else if (type === 'experimental') {
          // EXPERIMENTAL MODE 淡出
          // ⚠️ 暫時註解
          // element.classList.remove('active');
          // this.elements.experimentalIcon.src = '/static/assets/img_experimental_white.svg';

        } else if (type === 'card') {
          // 卡片淡出並隱藏
          element.style.opacity = '0';
          setTimeout(() => {
            element.classList.add('hidden');
            element.style.opacity = '1';
          }, 1600);
        }
      }, delay);
    });

    // === 6. 完成後恢復正常狀態 ===
    setTimeout(() => {
      document.body.classList.remove('startup-mode');

      // 恢復待機狀態
      this.showNoLead();
      this.elements.maxSpeed.textContent = '--';
      this.elements.vcruiseValue.textContent = '--';
      this.elements.speedLimitValue.textContent = '--';
      this.elements.upcomingValue.textContent = '--';
      this.elements.upcomingDistance.textContent = `--`;
      this.elements.cscSpeed.textContent = '--';
      this.elements.accelerationValue.textContent = '--';
      this.elements.accelerationValue.className = 'value-medium neutral';
      this.elements.roadNameText.textContent = '--';

      console.log('[Dashboard] ✓ Startup sequence completed');
    }, 5600); // 總時長約 5.6 秒
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

    // 更新顯示（移除單位，更緊湊）
    this.elements.leadDistance.textContent = `${Math.round(distance)}`;
    this.elements.desiredDistance.textContent = `${Math.round(desiredDist)}`;
    this.elements.leadSpeed.textContent = `${Math.round(leadSpeed)}`;
    this.elements.followTime.textContent = followTime !== '--' ? `${followTime}` : '--';

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
    // 保留 vEgo 狀態供跟車時間計算使用

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

    const maxSpeed = vCruise;  // vCruise 已經是 KPH，不需要轉換
    this.elements.maxSpeed.textContent = Math.round(maxSpeed);
  }

  // ========== 速限標誌更新 ==========

  updateSpeedLimit(frogpilotPlan) {
    // 使用 MAPD 速限（與 openpilot UI 和 drive_helpers 一致）
    // 這是控制定速的統一資料來源 (unified data source)
    const speedLimit = frogpilotPlan.mapdSpeedLimit || 0;  // m/s

    // 永遠顯示速限，沒有數據時顯示 --
    if (speedLimit <= 0) {
      this.elements.speedLimitValue.textContent = '--';
    } else {
      // 直接轉換單位 (m/s -> km/h 或 mph)
      const speedLimitConverted = speedLimit * this.units.speedConversion;
      this.elements.speedLimitValue.textContent = Math.round(speedLimitConverted).toString();
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
      this.elements.upcomingDistance.textContent = `${Math.round(distanceConverted)}`;
    } else {
      this.elements.upcomingDistance.textContent = `--`;
    }
  }

  // ========== CSC 彎道速度控制更新 ==========

  updateCSC(frogpilotPlan) {
    const cscControlling = frogpilotPlan.cscControllingSpeed || false;
    const cscSpeed = frogpilotPlan.cscSpeed || 0;

    // 速度顯示 - CSC 控制時顯示速度，否則顯示 --
    if (cscSpeed > 0 && cscControlling) {
      const cscSpeedConverted = cscSpeed * this.units.speedConversion;
      this.elements.cscSpeed.textContent = Math.round(cscSpeedConverted);
    } else {
      this.elements.cscSpeed.textContent = '--';
    }
  }

  // ========== CEM 狀態更新 ==========

  updateCEM(frogpilotPlan) {
    const conditionalStatus = frogpilotPlan.ceStatus || 0;
    const experimentalMode = frogpilotPlan.experimentalMode || false;

    // 當未啟用且狀態為 0 時隱藏
    if (!experimentalMode && conditionalStatus === 0) {
      this.elements.cemCard.classList.add('hidden');
      return;
    }

    this.elements.cemCard.classList.remove('hidden');

    // 根據狀態選擇圖標和文字
    let icon = '🤖';
    let text = '實驗';

    if (conditionalStatus === 1) {
      icon = '😌';
      text = '已暫停';
    } else if (conditionalStatus === 2 || experimentalMode) {
      icon = '🚀';
      text = '實驗';
    } else if (conditionalStatus === 3 || conditionalStatus === 4) {
      icon = '⚡';
      text = '低速';
    } else if (conditionalStatus === 5) {
      icon = '↪️';
      text = '轉彎';
    } else if (conditionalStatus === 6 || conditionalStatus === 7) {
      icon = '↪️';
      text = '導航';
    } else if (conditionalStatus === 8) {
      icon = '🌀';
      text = '彎道';
    } else if (conditionalStatus === 9 || conditionalStatus === 10) {
      icon = '🚗';
      text = '跟車';
    } else if (conditionalStatus === 11 || conditionalStatus === 12) {
      icon = '🚦';
      text = '號誌';
    } else if (conditionalStatus === 13) {
      icon = '🚀';
      text = '速限';
    }

    this.elements.cemIcon.textContent = icon;
    this.elements.cemText.textContent = text;
  }

  // ========== 加減速度更新 ==========

  updateAcceleration(aEgo) {
    if (aEgo === undefined || aEgo === null) {
      this.elements.accelerationValue.textContent = '--';
      this.elements.accelerationValue.className = 'value-medium neutral';
      return;
    }

    // 顯示加速度數值 (保留 2 位小數，使用絕對值不顯示負號)
    this.elements.accelerationValue.textContent = Math.abs(aEgo).toFixed(2);

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

    // 單位已從 HTML 移除，不需要更新顯示
  }

  // ========== VCRUISE 更新 ==========

  /**
   * 更新 vCruise 顯示
   * @param {Object} frogpilotPlan - FrogPilot plan 資料
   */
  updateVCruise(frogpilotPlan) {
    if (!frogpilotPlan || !frogpilotPlan.vCruise || frogpilotPlan.vCruise <= 0) {
      this.elements.vcruiseValue.textContent = '--';
      return;
    }

    // m/s 轉換為 km/h 或 mph
    const vCruise = frogpilotPlan.vCruise * this.units.speedConversion;
    this.elements.vcruiseValue.textContent = Math.round(vCruise).toString();
  }

  // ========== EXPERIMENTAL MODE 更新 ==========

  updateExperimentalMode(frogpilotPlan) {
    if (!frogpilotPlan) {
      // 沒訊號：顯示未啟動圖示
      this.elements.experimentalIcon.src = '/static/assets/img_experimental_white.svg';
      this.elements.experimentalCard.classList.remove('active');
      return;
    }

    // 根據 experimentalMode 切換圖示
    const isExperimental = frogpilotPlan.experimentalMode || false;

    if (isExperimental) {
      // 啟動：顯示橙色圖示
      this.elements.experimentalIcon.src = '/static/assets/img_experimental.svg';
      this.elements.experimentalCard.classList.add('active');
    } else {
      // 未啟動：顯示白色圖示
      this.elements.experimentalIcon.src = '/static/assets/img_experimental_white.svg';
      this.elements.experimentalCard.classList.remove('active');
    }
  }

  // ========== 連接狀態更新 ==========

  /**
   * 更新連接狀態顯示
   * @param {Boolean} connected - 是否已連接
   */
  updateConnectionStatus(connected) {
    if (connected) {
      this.elements.statusText.textContent = '已連接';
      this.elements.connectionStatus.className = 'connection-badge connected';
    } else {
      this.elements.statusText.textContent = '連接中...';
      this.elements.connectionStatus.className = 'connection-badge connecting';
    }
  }

  // ========== 盲點警示更新 ==========

  /**
   * 更新盲點警示顯示
   * @param {Object} carState - 車輛狀態資料
   */
  updateBlindspots(carState) {
    if (!carState) {
      this.elements.leftBlindspot.classList.remove('active');
      this.elements.rightBlindspot.classList.remove('active');
      return;
    }

    // 左側盲點
    if (carState.leftBlindspot) {
      this.elements.leftBlindspot.classList.remove('hidden');
      this.elements.leftBlindspot.classList.add('active');
    } else {
      this.elements.leftBlindspot.classList.remove('active');
    }

    // 右側盲點
    if (carState.rightBlindspot) {
      this.elements.rightBlindspot.classList.remove('hidden');
      this.elements.rightBlindspot.classList.add('active');
    } else {
      this.elements.rightBlindspot.classList.remove('active');
    }
  }

  // ========== 速限標誌樣式切換 ==========

  setSpeedLimitStyle(isVienna) {
    this.state.isViennaSign = isVienna;
  }
}
