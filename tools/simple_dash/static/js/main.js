/**
 * FrogPilot Dashboard 主程式
 * 整合 WebRTC 和 Dashboard UI
 */

import { start, stop } from './webrtc.js';
import { DashboardUI } from './dashboard.js';

// ==================== 狀態管理 ====================

const stateManager = {
  carState: null,          // 基本車輛狀態 (vEgo, aEgo, vCruise 等)
  radarState: null,        // 雷達狀態 (前車資訊)
  frogpilotPlan: null      // FrogPilot 路徑規劃 (速限, 彎道控制等)
};

// ==================== 初始化 Dashboard ====================

const dashboard = new DashboardUI();

// ==================== WebRTC 訊息處理 ====================

function handleMessage(msgType, msgData) {
  // 更新狀態管理器
  if (stateManager.hasOwnProperty(msgType)) {
    stateManager[msgType] = msgData;
  }

  // 根據訊息類型更新對應的 UI
  switch(msgType) {
    case 'carState':
      // 更新當前速度
      if (msgData.vEgo !== undefined) {
        dashboard.updateCurrentSpeed(msgData.vEgo);
      }
      // 更新加減速度
      if (msgData.aEgo !== undefined) {
        dashboard.updateAcceleration(msgData.aEgo);
      }
      // 更新 MAX 速度
      if (msgData.cruiseState) {
        const vCruise = msgData.cruiseState.speed || msgData.vEgoCluster || 0;
        dashboard.updateMaxSpeed(vCruise);
      }
      break;

    case 'frogpilotPlan':
      // 更新速限標誌
      dashboard.updateSpeedLimit(msgData);
      // 更新 CSC 彎道控制
      dashboard.updateCSC(msgData);
      // 更新道路名稱
      if (msgData.roadName !== undefined) {
        dashboard.updateRoadName(msgData.roadName);
      }
      // 更新前車資訊 (需要配合 radarState)
      if (stateManager.radarState) {
        dashboard.updateLeadInfo(stateManager.radarState, msgData);
      }
      break;

    case 'radarState':
      // 更新前車資訊
      dashboard.updateLeadInfo(msgData, stateManager.frogpilotPlan);
      break;

    // 未來可以擴展更多服務
    default:
      // 其他訊息類型暫時不處理
      break;
  }

  // 除錯訊息 (可選)
  // console.log(`[${msgType}]`, msgData);
}

// ==================== 參數讀取 (模擬從 params_memory 讀取) ====================

// 注意: 這裡需要實際從 params_memory 讀取，目前先用模擬資料
function loadParams() {
  // 單位設定 (公制/英制)
  const isMetric = true;  // 從 params 讀取
  dashboard.setMetric(isMetric);

  // 速限標誌樣式 (Vienna/MUTCD)
  const isVienna = false;  // 從 params 讀取
  dashboard.setSpeedLimitStyle(isVienna);

  console.log('[Params] Loaded: metric=' + isMetric + ', vienna=' + isVienna);
}

// ==================== 初始化 ====================

async function init() {
  console.log('🚗 Initializing FrogPilot Dashboard...');

  try {
    // 載入參數
    loadParams();

    // 啟動 WebRTC 連接
    console.log('Connecting to WebRTC...');
    dashboard.updateConnectionStatus(false, '連接中...');

    await start(handleMessage);

    console.log('✓ WebRTC connected (Data Channel only)');
    dashboard.updateConnectionStatus(true, '已連接');

    console.log('✓ FrogPilot Dashboard initialized');
    console.log('');
    console.log('訂閱的服務（最小化訂閱，減少系統負擔）:');
    console.log('- carState (基本車輛狀態: 速度, 加速度, cruise 等)');
    console.log('- frogpilotPlan (FrogPilot 路徑規劃: 速限, 彎道控制等)');
    console.log('- radarState (雷達狀態: 前車資訊)');

  } catch (error) {
    console.error('❌ Failed to initialize:', error);
    dashboard.updateConnectionStatus(false, '連接失敗');

    alert('連接失敗，請確認:\n1. webrtcd 正在運行 (port 5001)\n2. 網路連接正常\n3. simple_dash 已啟動 (port 8000)');
  }
}

// ==================== 清理 ====================

window.addEventListener('beforeunload', () => {
  console.log('Shutting down...');
  stop();
});

// ==================== 啟動 ====================

// 等待 DOM 載入完成
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', init);
} else {
  init();
}
