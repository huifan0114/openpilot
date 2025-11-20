/**
 * Vision Monitor - 視覺不確定性監控
 * 獨立頁面，實時顯示視覺模型的不確定性指標
 */

// ==================== VisionMonitor 類別 ====================

class VisionMonitor {
  constructor() {
    // WebRTC 連接
    this.pc = null;
    this.dc = null;

    // 狀態數據
    this.stateManager = {
      modelV2: null,
      carState: null,
      controlsState: null
    };

    // vRel_stuck 偵測用歷史數據
    this.vRelHistory = [];
    this.dRelHistory = [];
    this.historySize = 50;  // 約 2.5 秒 (20Hz)

    // 初始化 DOM 元素
    this.initElements();

    // 啟動連接
    this.connect();
  }

  // ========== DOM 元素初始化 ==========

  initElements() {
    this.elements = {
      // 連接狀態
      connectionStatus: document.getElementById('connection-status'),
      lastUpdate: document.getElementById('last-update'),

      // 安全指標
      ttc: document.getElementById('ttc'),
      th: document.getElementById('th'),
      drel: document.getElementById('drel'),
      vrel: document.getElementById('vrel'),
      vrelStuck: document.getElementById('vrel-stuck'),
      ttcWarn: document.getElementById('ttc-warn'),
      thWarn: document.getElementById('th-warn'),
      vrelStuckWarn: document.getElementById('vrel-stuck-warn'),

      // 定速模擬
      vcruiseOriginal: document.getElementById('vcruise-original'),
      vcruiseSuggested: document.getElementById('vcruise-suggested'),
      vcruiseReduction: document.getElementById('vcruise-reduction'),
      simStatus: document.getElementById('sim-status'),

      // 觸發條件標籤
      triggerTags: {
        ttc: document.getElementById('tag-ttc'),
        th: document.getElementById('tag-th'),
        vrelStuck: document.getElementById('tag-vrel-stuck'),
        accel: document.getElementById('tag-accel')
      }
    };
  }

  // ========== WebRTC 連接管理 ==========

  async connect() {
    console.log('🔌 開始連接 WebRTC...');
    this.updateConnectionStatus(false, '連接中...');

    try {
      // 建立 PeerConnection
      this.pc = this.createPeerConnection();

      // 建立 Data Channel
      const parameters = { ordered: true };
      this.dc = this.pc.createDataChannel('data', parameters);

      // Data Channel 事件處理
      this.dc.onopen = () => {
        console.log('✓ Data channel opened');
        this.updateConnectionStatus(true);
      };

      this.dc.onclose = () => {
        console.log('✗ Data channel closed');
        this.updateConnectionStatus(false, '已斷開');
      };

      this.dc.onerror = (error) => {
        console.error('✗ Data channel error:', error);
        this.updateConnectionStatus(false, '連接錯誤');
      };

      // 處理接收到的訊息
      const textDecoder = new TextDecoder();
      this.dc.onmessage = (evt) => {
        try {
          const text = textDecoder.decode(evt.data);
          const msg = JSON.parse(text);

          if (msg.type && msg.data) {
            this.handleMessage(msg.type, msg.data);
          }
        } catch (e) {
          console.error('Failed to parse message:', e);
        }
      };

      // 開始 SDP 協商
      await this.negotiate();
      console.log('✓ WebRTC 連接建立成功');

    } catch (error) {
      console.error('❌ WebRTC 連接失敗:', error);
      this.updateConnectionStatus(false, '連接失敗');
      alert('連接失敗，請確認:\n1. webrtcd 正在運行 (port 5001)\n2. 網路連接正常\n3. simple_dash 已啟動 (port 8000)');
    }
  }

  createPeerConnection() {
    const config = {
      sdpSemantics: 'unified-plan'
    };

    return new RTCPeerConnection(config);
  }

  async negotiate() {
    // Data Channel only - 不接收音訊/視訊
    const offer = await this.pc.createOffer({
      offerToReceiveAudio: false,
      offerToReceiveVideo: false
    });

    await this.pc.setLocalDescription(offer);

    // 等待 ICE 候選收集完成
    await new Promise((resolve) => {
      if (this.pc.iceGatheringState === 'complete') {
        resolve();
      } else {
        const checkState = () => {
          if (this.pc.iceGatheringState === 'complete') {
            this.pc.removeEventListener('icegatheringstatechange', checkState);
            resolve();
          }
        };
        this.pc.addEventListener('icegatheringstatechange', checkState);
      }
    });

    // 發送 offer 到 webrtcd
    const localDesc = this.pc.localDescription;
    const response = await this.offerRtcRequest(localDesc.sdp, localDesc.type);

    if (!response.ok) {
      const errorData = await response.json();
      console.error('[ERROR] Server returned error:', errorData);
      throw new Error(`Server error: ${errorData.error}`);
    }

    const answer = await response.json();

    if (!answer.sdp || !answer.type) {
      throw new Error(`Invalid answer: missing ${!answer.sdp ? 'sdp' : 'type'} field`);
    }

    await this.pc.setRemoteDescription(answer);
  }

  offerRtcRequest(sdp, type) {
    const body = {
      sdp: sdp,
      cameras: [],  // Data Channel only
      bridge_services_in: [],
      bridge_services_out: [
        "modelV2",          // 視覺模型輸出（前車不確定性）
        "carState",         // 車輛狀態（速度、加速度）
        "controlsState"     // 控制狀態（定速設定值）
      ]
    };

    const host = window.location.hostname || 'localhost';
    const url = `http://${host}:5001/stream`;

    return fetch(url, {
      body: JSON.stringify(body),
      headers: { 'Content-Type': 'application/json' },
      method: 'POST'
    });
  }

  disconnect() {
    if (this.dc) {
      this.dc.close();
      this.dc = null;
    }

    if (this.pc) {
      if (this.pc.getTransceivers) {
        this.pc.getTransceivers().forEach(transceiver => {
          if (transceiver.stop) {
            transceiver.stop();
          }
        });
      }

      this.pc.getSenders().forEach(sender => {
        if (sender.track) {
          sender.track.stop();
        }
      });

      setTimeout(() => {
        this.pc.close();
        this.pc = null;
      }, 500);
    }

    this.updateConnectionStatus(false, '已斷開');
  }

  // ========== 訊息處理 ==========

  handleMessage(msgType, msgData) {
    // 更新狀態管理器
    if (this.stateManager.hasOwnProperty(msgType)) {
      this.stateManager[msgType] = msgData;
    }

    // 當收到 modelV2 時觸發更新（需要配合 carState 和 controlsState）
    if (msgType === 'modelV2') {
      this.updateMonitor();
    }
  }

  // ========== 主更新函數 ==========

  updateMonitor() {
    const modelV2 = this.stateManager.modelV2;
    const carState = this.stateManager.carState;
    const controlsState = this.stateManager.controlsState;

    // 檢查是否有必要數據
    if (!modelV2 || !modelV2.leadsV3 || modelV2.leadsV3.length === 0) {
      this.showNoData();
      return;
    }

    if (!carState) {
      return;
    }

    const lead = modelV2.leadsV3[0];
    const vEgo = carState.vEgo || 0;

    // 更新安全指標（包含 TTC 和 TH）
    this.updateSafety(lead, vEgo);

    // 更新定速模擬
    if (controlsState) {
      this.updateCruiseSim(lead, vEgo, controlsState, carState.aEgo || 0);
    }

    // 更新最後更新時間
    this.updateLastUpdate();
  }

  // ========== 安全指標更新 ==========

  updateSafety(lead, vEgo) {
    // 距離和速度
    const dRel = (lead.x && lead.x.length > 0) ? lead.x[0] : 0;
    const vLead = (lead.v && lead.v.length > 0) ? lead.v[0] : 0;
    const vRel = vLead - vEgo;

    // 顯示距離
    this.elements.drel.textContent = dRel.toFixed(1);

    // 顯示相對速度 (km/h)
    const vRelKph = vRel * 3.6;
    this.elements.vrel.textContent = vRelKph.toFixed(1);

    // TTC (碰撞時間)
    let ttc = 999;
    if (vRel > 0.1) {  // 正在接近
      ttc = dRel / vRel;
    }

    // TH (Time Headway 車頭時距)
    let th = 999;
    if (vEgo > 0.1) {
      th = dRel / vEgo;
    }

    // 顯示 TTC（閾值調嚴格）
    const ttcElem = this.elements.ttc;
    if (ttc > 99) {
      ttcElem.textContent = '--';
      ttcElem.classList.remove('ttc-danger', 'ttc-warning', 'ttc-caution');
      this.elements.ttcWarn.classList.add('hidden');
    } else {
      ttcElem.textContent = ttc.toFixed(1);

      // TTC 顏色分級和警告
      ttcElem.classList.remove('ttc-danger', 'ttc-warning', 'ttc-caution');
      if (ttc < 2.5) {
        ttcElem.classList.add('ttc-danger');
        this.elements.ttcWarn.classList.remove('hidden');
      } else if (ttc < 4) {
        ttcElem.classList.add('ttc-warning');
        this.elements.ttcWarn.classList.remove('hidden');
      } else {
        ttcElem.classList.add('ttc-caution');
        this.elements.ttcWarn.classList.add('hidden');
      }
    }

    // 顯示 TH（閾值調嚴格）
    const thElem = this.elements.th;
    if (th > 99) {
      thElem.textContent = '--';
      thElem.classList.remove('th-danger', 'th-warning');
      this.elements.thWarn.classList.add('hidden');
    } else {
      thElem.textContent = th.toFixed(2);

      // TH 顏色分級和警告
      thElem.classList.remove('th-danger', 'th-warning');
      if (th < 0.8) {
        thElem.classList.add('th-danger');
        this.elements.thWarn.classList.remove('hidden');
      } else if (th < 1.2) {
        thElem.classList.add('th-warning');
        this.elements.thWarn.classList.remove('hidden');
      } else {
        this.elements.thWarn.classList.add('hidden');
      }
    }

    // 更新歷史數據並偵測 vRel_stuck
    this.updateHistory(vRel, dRel);
    const isVRelStuck = this.detectVRelStuck();

    // 顯示 vRel_stuck 狀態
    const vrelStuckElem = this.elements.vrelStuck;
    if (isVRelStuck) {
      vrelStuckElem.textContent = '異常';
      vrelStuckElem.classList.add('vrel-stuck-danger');
      this.elements.vrelStuckWarn.classList.remove('hidden');
    } else {
      vrelStuckElem.textContent = '正常';
      vrelStuckElem.classList.remove('vrel-stuck-danger');
      this.elements.vrelStuckWarn.classList.add('hidden');
    }
  }

  // ========== 歷史數據更新 ==========

  updateHistory(vRel, dRel) {
    this.vRelHistory.push(vRel);
    this.dRelHistory.push(dRel);
    if (this.vRelHistory.length > this.historySize) {
      this.vRelHistory.shift();
      this.dRelHistory.shift();
    }
  }

  // ========== vRel_stuck 偵測 ==========

  detectVRelStuck() {
    if (this.vRelHistory.length < this.historySize) return false;

    // 計算 vRel 標準差
    const mean = this.vRelHistory.reduce((a, b) => a + b, 0) / this.historySize;
    const variance = this.vRelHistory.reduce((sum, val) =>
      sum + Math.pow(val - mean, 2), 0) / this.historySize;
    const std = Math.sqrt(variance);

    // 計算 dRel 變化（第一個值 - 最後一個值，正值表示距離減少）
    const dRelChange = this.dRelHistory[0] - this.dRelHistory[this.historySize - 1];

    // 條件：vRel 標準差 < 0.5 m/s 且 dRel 變化 > 20m
    return std < 0.5 && dRelChange > 20;
  }

  // ========== 定速模擬更新 ==========

  updateCruiseSim(lead, vEgo, controlsState, aEgo) {
    // vCruise 已經是 km/h，不需要轉換
    const vCruiseKph = controlsState.vCruise || 0;
    const vEgoKph = vEgo * 3.6;

    // 顯示原始定速
    this.elements.vcruiseOriginal.textContent = vCruiseKph.toFixed(0);

    // 計算 TTC 和 TH
    const dRel = (lead.x && lead.x.length > 0) ? lead.x[0] : 0;
    const vLead = (lead.v && lead.v.length > 0) ? lead.v[0] : 0;
    const vRel = vLead - vEgo;

    let ttc = 999;
    if (vRel > 0.1) {
      ttc = dRel / vRel;
    }

    let th = 999;
    if (vEgo > 0.1) {
      th = dRel / vEgo;
    }

    // === 條件判斷 ===
    let triggers = {
      ttc: false,
      th: false,
      vrelStuck: false,
      accel: false
    };

    // 檢查各項條件（閾值調嚴格）
    if (ttc < 4 && ttc < 99) triggers.ttc = true;
    if (th < 1.2 && th < 99) triggers.th = true;
    triggers.vrelStuck = this.detectVRelStuck();

    // 危險加速場景
    const accelerating = aEgo > 0.5;
    const largeSpeedGap = (vCruiseKph - vEgoKph) > 20;
    if (accelerating && largeSpeedGap && th < 2.0) {
      triggers.accel = true;
    }

    // === 風險等級判斷 ===
    let riskLevel = 'normal';  // normal, medium, high
    let suggestedVCruiseKph = vCruiseKph;

    // 高風險：TTC < 2.5 或 TH < 0.8 或 vRel_stuck
    const highRisk = (ttc < 2.5 && ttc < 99) || (th < 0.8 && th < 99) || triggers.vrelStuck;

    // 中風險：TTC < 4 或 TH < 1.2 或危險加速
    const mediumRisk = triggers.ttc || triggers.th || triggers.accel;

    if (highRisk) {
      riskLevel = 'high';
      // 降到當前速度下方最近的 5 倍數
      suggestedVCruiseKph = Math.floor(vEgoKph / 5) * 5;
      suggestedVCruiseKph = Math.max(suggestedVCruiseKph, 30);  // 最低 30 km/h

    } else if (mediumRisk) {
      riskLevel = 'medium';
      // 降到當前速度 + 10 km/h
      suggestedVCruiseKph = Math.min(vCruiseKph, vEgoKph + 10);
      suggestedVCruiseKph = Math.max(suggestedVCruiseKph, 30);
    }

    // === 顯示建議定速 ===
    this.elements.vcruiseSuggested.textContent = suggestedVCruiseKph.toFixed(0);

    // 設置建議定速顏色
    this.elements.vcruiseSuggested.classList.remove('sim-normal', 'sim-warning', 'sim-danger');
    if (riskLevel === 'high') {
      this.elements.vcruiseSuggested.classList.add('sim-danger');
    } else if (riskLevel === 'medium') {
      this.elements.vcruiseSuggested.classList.add('sim-warning');
    } else {
      this.elements.vcruiseSuggested.classList.add('sim-normal');
    }

    // === 顯示降低幅度 ===
    const reduction = vCruiseKph - suggestedVCruiseKph;
    if (reduction > 0.5) {
      this.elements.vcruiseReduction.textContent = '-' + reduction.toFixed(0);
      this.elements.vcruiseReduction.classList.add('reduction-active');
    } else {
      this.elements.vcruiseReduction.textContent = '--';
      this.elements.vcruiseReduction.classList.remove('reduction-active');
    }

    // === 顯示狀態徽章 ===
    const statusBadge = this.elements.simStatus;
    statusBadge.classList.remove('normal', 'warning', 'danger');

    if (riskLevel === 'high') {
      statusBadge.textContent = '降速中';
      statusBadge.classList.add('danger');
    } else if (riskLevel === 'medium') {
      statusBadge.textContent = '警告';
      statusBadge.classList.add('warning');
    } else {
      statusBadge.textContent = '正常';
      statusBadge.classList.add('normal');
    }

    // === 顯示觸發條件標籤 ===
    for (const [key, isTriggered] of Object.entries(triggers)) {
      const tag = this.elements.triggerTags[key];
      if (tag) {
        if (isTriggered) {
          tag.classList.remove('hidden');
        } else {
          tag.classList.add('hidden');
        }
      }
    }
  }

  // ========== 輔助函數 ==========

  updateConnectionStatus(connected, text = null) {
    const statusElem = this.elements.connectionStatus;

    if (connected) {
      statusElem.textContent = text || '已連接';
      statusElem.classList.remove('disconnected');
      statusElem.classList.add('connected');
    } else {
      statusElem.textContent = text || '未連接';
      statusElem.classList.remove('connected');
      statusElem.classList.add('disconnected');
    }
  }

  updateLastUpdate() {
    const now = new Date();
    const timeStr = now.toLocaleTimeString('zh-TW', {
      hour12: false,
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit'
    });
    this.elements.lastUpdate.textContent = `最後更新: ${timeStr}`;
  }

  showNoData() {
    // 無前車數據時顯示 "--"
    this.elements.ttc.textContent = '--';
    this.elements.th.textContent = '--';
    this.elements.drel.textContent = '--';
    this.elements.vrel.textContent = '--';
    this.elements.vrelStuck.textContent = '--';
    this.elements.vcruiseOriginal.textContent = '--';
    this.elements.vcruiseSuggested.textContent = '--';
    this.elements.vcruiseReduction.textContent = '--';

    // 隱藏所有警告標記
    this.elements.ttcWarn.classList.add('hidden');
    this.elements.thWarn.classList.add('hidden');
    this.elements.vrelStuckWarn.classList.add('hidden');

    // 隱藏所有觸發標籤
    for (const tag of Object.values(this.elements.triggerTags)) {
      if (tag) {
        tag.classList.add('hidden');
      }
    }

    // 重置狀態
    this.elements.simStatus.textContent = '等待數據';
    this.elements.simStatus.classList.remove('normal', 'warning', 'danger');

    // 清空歷史數據
    this.vRelHistory = [];
    this.dRelHistory = [];
  }
}

// ==================== 初始化 ====================

let monitor = null;

function init() {
  console.log('🚀 Vision Monitor 初始化中...');
  monitor = new VisionMonitor();
  console.log('✓ Vision Monitor 已啟動');
  console.log('');
  console.log('訂閱的服務:');
  console.log('- modelV2 (視覺模型輸出)');
  console.log('- carState (車輛狀態)');
  console.log('- controlsState (控制狀態)');
}

// ==================== 清理 ====================

window.addEventListener('beforeunload', () => {
  console.log('Vision Monitor 關閉中...');
  if (monitor) {
    monitor.disconnect();
  }
});

// ==================== 啟動 ====================

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', init);
} else {
  init();
}
