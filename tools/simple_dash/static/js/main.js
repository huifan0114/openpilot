/**
 * FrogPilot Dashboard 主程式
 * 整合 WebRTC 和 Dashboard UI
 */

import { start, stop } from './webrtc.js';
import { DashboardUI } from './dashboard.js';

// ==================== 狀態管理 ====================

const stateManager = {
  // openpilot 原始服務
  modelV2: null,
  liveCalibration: null,
  carState: null,
  controlsState: null,
  radarState: null,

  // FrogPilot 特有服務
  frogpilotModelV2: null,
  frogpilotCarState: null,
  frogpilotControlsState: null,
  frogpilotPlan: null,
  frogpilotRadarState: null
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
      // 更新踏板 (需要加速度)
      if (msgData.aEgo !== undefined) {
        dashboard.updatePedals(msgData.aEgo, stateManager.frogpilotCarState?.brakeLights || false);
      }
      break;

    case 'frogpilotCarState':
      // 更新踏板 (煞車燈狀態)
      if (msgData.brakeLights !== undefined && stateManager.carState) {
        dashboard.updatePedals(stateManager.carState.aEgo || 0, msgData.brakeLights);
      }
      break;

    case 'controlsState':
      // 更新 MAX 速度
      if (msgData.vCruise !== undefined || msgData.vCruiseCluster !== undefined) {
        const vCruise = msgData.vCruiseCluster || msgData.vCruise;
        dashboard.updateMaxSpeed(vCruise);
      }
      // 更新 CEM (需要配合 frogpilotPlan)
      if (stateManager.frogpilotPlan) {
        dashboard.updateCEM(stateManager.frogpilotPlan, msgData);
      }
      break;

    case 'frogpilotPlan':
      // 更新速限標誌
      dashboard.updateSpeedLimit(msgData);
      // 更新 CSC 彎道控制
      dashboard.updateCSC(msgData);
      // 更新 CEM 狀態
      if (stateManager.controlsState) {
        dashboard.updateCEM(msgData, stateManager.controlsState);
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

    case 'frogpilotRadarState':
      // 如果有 FrogPilot 版本的雷達狀態，優先使用
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
  // 道路名稱 (從 params_memory 讀取)
  // dashboard.updateRoadName('中山北路');

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
    console.log('訂閱的服務:');
    console.log('- carState, frogpilotCarState');
    console.log('- controlsState, frogpilotControlsState');
    console.log('- radarState, frogpilotRadarState');
    console.log('- frogpilotPlan');
    console.log('- modelV2, frogpilotModelV2, liveCalibration');

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
