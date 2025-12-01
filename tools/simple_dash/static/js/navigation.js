/**
 * NavBridge 導航模組
 * 從 Android NavBridge App 取得 Google Maps 導航資料
 */

// ==================== 設定 ====================

// NavBridge App 的 IP 和 Port
// 預設使用同一台裝置 (localhost)，可以在 localStorage 中設定其他 IP
const DEFAULT_NAVBRIDGE_HOST = 'localhost';
const NAVBRIDGE_PORT = 5002;

// 更新間隔 (毫秒)
const UPDATE_INTERVAL = 1000;

// ==================== 轉向圖示對應 ====================

const MANEUVER_ICONS = {
  // 轉向
  'turn_right': '↱',
  'turn_left': '↰',
  'turn_slight_right': '↗',
  'turn_slight_left': '↖',
  'turn_sharp_right': '⤵',
  'turn_sharp_left': '⤴',
  'turn_uturn': '↩',
  'turn_straight': '↑',

  // 高速公路
  'off_ramp_right': '⤷',
  'off_ramp_left': '⤶',
  'on_ramp_right': '⤴',
  'on_ramp_left': '⤴',

  // 岔路
  'fork_right': '⤳',
  'fork_left': '⤲',

  // 合流
  'merge_straight': '⤨',
  'merge_right': '⤨',
  'merge_left': '⤨',

  // 圓環
  'roundabout_straight': '↻',
  'roundabout_right': '↻',
  'roundabout_left': '↻',

  // 直行
  'straight_straight': '↑',

  // 抵達
  'arrive_straight': '🏁',

  // 預設
  'default': '→'
};

// ==================== 狀態 ====================

let navBridgeHost = localStorage.getItem('navBridgeHost') || DEFAULT_NAVBRIDGE_HOST;
let updateTimer = null;
let lastNavData = null;
let isConnected = false;

// ==================== DOM 元素 ====================

const elements = {
  navCard: null,
  navIcon: null,
  navDistance: null,
  navInstruction: null
};

// ==================== 初始化 ====================

/**
 * 初始化導航模組
 */
export function initNavigation() {
  console.log('[Navigation] Initializing...');

  // 取得 DOM 元素
  elements.navCard = document.getElementById('nav-instruction-card');
  elements.navIcon = document.getElementById('nav-icon');
  elements.navDistance = document.getElementById('nav-distance');
  elements.navInstruction = document.getElementById('nav-instruction');

  // 從 URL 參數讀取 NavBridge IP
  const urlParams = new URLSearchParams(window.location.search);
  const navIp = urlParams.get('navbridge');
  if (navIp) {
    navBridgeHost = navIp;
    localStorage.setItem('navBridgeHost', navIp);
    console.log(`[Navigation] Using NavBridge IP from URL: ${navIp}`);
  }

  console.log(`[Navigation] NavBridge host: ${navBridgeHost}:${NAVBRIDGE_PORT}`);

  // 啟動更新循環
  startUpdateLoop();
}

/**
 * 設定 NavBridge 主機位址
 */
export function setNavBridgeHost(host) {
  navBridgeHost = host;
  localStorage.setItem('navBridgeHost', host);
  console.log(`[Navigation] NavBridge host set to: ${host}`);
}

/**
 * 取得目前 NavBridge 主機位址
 */
export function getNavBridgeHost() {
  return navBridgeHost;
}

// ==================== 更新循環 ====================

/**
 * 啟動更新循環
 */
function startUpdateLoop() {
  if (updateTimer) {
    clearInterval(updateTimer);
  }

  // 立即執行一次
  fetchNavigation();

  // 定時更新
  updateTimer = setInterval(fetchNavigation, UPDATE_INTERVAL);
  console.log('[Navigation] Update loop started');
}

/**
 * 停止更新循環
 */
export function stopNavigation() {
  if (updateTimer) {
    clearInterval(updateTimer);
    updateTimer = null;
  }
  console.log('[Navigation] Update loop stopped');
}

// ==================== 資料取得 ====================

/**
 * 從 NavBridge 取得導航資料
 */
async function fetchNavigation() {
  try {
    const url = `http://${navBridgeHost}:${NAVBRIDGE_PORT}/nav`;
    const response = await fetch(url, {
      method: 'GET',
      mode: 'cors',
      cache: 'no-cache',
      signal: AbortSignal.timeout(2000)  // 2 秒超時
    });

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    const data = await response.json();
    isConnected = true;
    updateNavDisplay(data);
    lastNavData = data;

  } catch (error) {
    // 連線失敗
    if (isConnected) {
      console.warn('[Navigation] Connection lost:', error.message);
      isConnected = false;
    }
    hideNavigation();
  }
}

// ==================== UI 更新 ====================

/**
 * 更新導航顯示
 */
function updateNavDisplay(data) {
  if (!elements.navCard) return;

  // 檢查是否正在導航
  if (!data.isNavigating) {
    hideNavigation();
    return;
  }

  // 顯示導航卡片
  elements.navCard.classList.remove('hidden');

  // 道路名稱卡片保持顯示 (由 dashboard.js 控制)

  // 更新圖示
  const iconKey = `${data.maneuverType}_${data.maneuverModifier}`;
  const icon = MANEUVER_ICONS[iconKey] ||
               MANEUVER_ICONS[data.maneuverModifier] ||
               MANEUVER_ICONS['default'];
  elements.navIcon.textContent = icon;

  // 根據轉向方向設定圖示顏色
  updateIconStyle(data.maneuverModifier);

  // 更新距離
  elements.navDistance.textContent = formatDistance(data.distanceToNext);

  // 更新指示文字
  elements.navInstruction.textContent = data.instruction || '--';

  // 根據距離更新緊急程度
  updateUrgency(data.distanceToNext);
}

/**
 * 隱藏導航顯示
 */
function hideNavigation() {
  if (!elements.navCard) return;

  elements.navCard.classList.add('hidden');

  // 道路名稱卡片保持顯示 (由 dashboard.js 控制)
}

/**
 * 更新圖示樣式 (根據左/右方向)
 */
function updateIconStyle(modifier) {
  if (!elements.navIcon) return;

  // 移除舊的方向類別
  elements.navIcon.classList.remove('nav-left', 'nav-right', 'nav-uturn');

  // 根據方向添加類別
  if (modifier && modifier.includes('left')) {
    elements.navIcon.classList.add('nav-left');
  } else if (modifier && modifier.includes('right')) {
    elements.navIcon.classList.add('nav-right');
  } else if (modifier && modifier.includes('uturn')) {
    elements.navIcon.classList.add('nav-uturn');
  }
}

/**
 * 更新緊急程度 (根據距離)
 */
function updateUrgency(distance) {
  if (!elements.navCard) return;

  // 移除舊的緊急程度類別
  elements.navCard.classList.remove('nav-urgent', 'nav-warning', 'nav-normal');

  if (distance <= 100) {
    // 非常近 - 緊急 (紅色)
    elements.navCard.classList.add('nav-urgent');
  } else if (distance <= 300) {
    // 接近 - 警告 (橙色)
    elements.navCard.classList.add('nav-warning');
  } else {
    // 一般 - 正常 (綠色)
    elements.navCard.classList.add('nav-normal');
  }
}

// ==================== 工具函數 ====================

/**
 * 格式化距離
 */
function formatDistance(meters) {
  if (!meters || meters <= 0) return '--';

  if (meters >= 1000) {
    return `${(meters / 1000).toFixed(1)} km`;
  } else {
    return `${Math.round(meters)} m`;
  }
}

// ==================== 匯出 ====================

export { lastNavData, isConnected };

// 自動初始化
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initNavigation);
} else {
  initNavigation();
}
