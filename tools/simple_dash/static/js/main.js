/**
 * AR HUD 主程式
 * 整合 WebRTC、Canvas 渲染器和 AR 控制器
 */

import { start, stop } from './webrtc.js';
import { MainRenderer } from './renderers.js';
import { ARController } from './ar-controls.js';

// ==================== 狀態管理 ====================

const stateManager = {
  modelV2: null,
  liveCalibration: null,
  carState: null,
  controlsState: null,
  selfdriveState: null
};

// ==================== 初始化模組 ====================

// 渲染器
const renderer = new MainRenderer();

// AR 控制器
const arController = new ARController();

// DOM 元素
const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');
const container = document.getElementById('canvasContainer');

// ==================== Canvas 大小調整 ====================

function resizeCanvas() {
  const rect = container.getBoundingClientRect();

  // 只在尺寸改變時更新
  if (canvas.width !== rect.width || canvas.height !== rect.height) {
    canvas.width = rect.width;
    canvas.height = rect.height;
  }
}

// ==================== 渲染迴圈 ====================

let lastRenderTime = 0;
const TARGET_FPS = 30;
const FRAME_INTERVAL = 1000 / TARGET_FPS;

function renderLoop(timestamp) {
  // 控制幀率
  if (timestamp - lastRenderTime < FRAME_INTERVAL) {
    requestAnimationFrame(renderLoop);
    return;
  }
  lastRenderTime = timestamp;

  // 調整 canvas 大小
  resizeCanvas();

  // 清空畫布
  ctx.save();
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  // 傳遞 HUD 和 AR 模式狀態給渲染器
  renderer.setHUDMode(arController.isHUDMode());
  renderer.setARMode(arController.isARMode());

  // 渲染所有內容
  renderer.render(ctx, stateManager, canvas.width, canvas.height);

  ctx.restore();

  // 下一幀
  requestAnimationFrame(renderLoop);
}

// ==================== WebRTC 訊息處理 ====================

function handleMessage(msgType, msgData) {
  // 更新對應的狀態
  if (stateManager.hasOwnProperty(msgType)) {
    stateManager[msgType] = msgData;
  }

  // 除錯訊息 (可選)
  // if (msgType === 'modelV2' || msgType === 'carState') {
  //   console.log(`Received ${msgType}:`, msgData);
  // }
}

// ==================== 初始化 ====================

async function init() {
  console.log('🚗 Initializing AR HUD Dashboard...');

  try {
    // 啟動 WebRTC 連接
    console.log('Connecting to WebRTC...');
    await start(handleMessage);

    console.log('✓ WebRTC connected (Data Channel only)');

    // 不等待視訊，直接開始渲染
    console.log('🎨 Starting render loop...');
    requestAnimationFrame(renderLoop);

    // 視窗大小改變時重新調整
    window.addEventListener('resize', () => {
      resizeCanvas();
    });

    // 頁面關閉時清理
    window.addEventListener('beforeunload', () => {
      stop();
    });

    console.log('✓ AR HUD Dashboard initialized');
    console.log('');
    console.log('使用說明:');
    console.log('- AR Mode: 隱藏視訊背景');
    console.log('- HUD Mirror: Y 軸翻轉投影');
    console.log('- 單指拖曳: 移動顯示區域');
    console.log('- 雙指縮放: 調整大小');

  } catch (error) {
    console.error('❌ Failed to initialize:', error);
    alert('連接失敗，請確認:\n1. webrtcd 正在運行 (port 5001)\n2. 網路連接正常\n3. 瀏覽器支援 WebRTC');
  }
}

// ==================== 啟動 ====================

// 等待 DOM 載入完成
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', init);
} else {
  init();
}
