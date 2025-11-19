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

      // 不確定性指標
      xstd: document.getElementById('xstd'),
      vstd: document.getElementById('vstd'),
      prob: document.getElementById('prob'),
      xstdWarn: document.getElementById('xstd-warn'),
      vstdWarn: document.getElementById('vstd-warn'),
      probWarn: document.getElementById('prob-warn'),

      // 安全指標
      ttc: document.getElementById('ttc'),
      drel: document.getElementById('drel'),
      vrel: document.getElementById('vrel'),

      // 定速模擬
      vcruiseOriginal: document.getElementById('vcruise-original'),
      vcruiseSuggested: document.getElementById('vcruise-suggested'),
      vcruiseReduction: document.getElementById('vcruise-reduction'),
      simStatus: document.getElementById('sim-status'),

      // 觸發條件標籤
      triggerTags: {
        xstd: document.getElementById('tag-xstd'),
        vstd: document.getElementById('tag-vstd'),
        prob: document.getElementById('tag-prob'),
        ttc: document.getElementById('tag-ttc'),
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

    // 更新不確定性指標
    this.updateUncertainty(lead);

    // 更新安全指標
    this.updateSafety(lead, vEgo);

    // 更新定速模擬
    if (controlsState) {
      this.updateCruiseSim(lead, vEgo, controlsState, carState.aEgo || 0);
    }

    // 更新最後更新時間
    this.updateLastUpdate();
  }

  // ========== 不確定性指標更新 ==========

  updateUncertainty(lead) {
    // xStd (距離標準差)
    const xStd = (lead.xStd && lead.xStd.length > 0) ? lead.xStd[0] : 0;
    this.elements.xstd.textContent = xStd.toFixed(2);

    // xStd 超過閾值 (2.17m)
    if (xStd > 2.17) {
      this.elements.xstd.classList.add('warn-value');
      this.elements.xstdWarn.classList.remove('hidden');
    } else {
      this.elements.xstd.classList.remove('warn-value');
      this.elements.xstdWarn.classList.add('hidden');
    }

    // vStd (速度標準差)
    const vStd = (lead.vStd && lead.vStd.length > 0) ? lead.vStd[0] : 0;
    this.elements.vstd.textContent = vStd.toFixed(2);

    // vStd 超過閾值 (1.08 m/s)
    if (vStd > 1.08) {
      this.elements.vstd.classList.add('warn-value');
      this.elements.vstdWarn.classList.remove('hidden');
    } else {
      this.elements.vstd.classList.remove('warn-value');
      this.elements.vstdWarn.classList.add('hidden');
    }

    // Prob (前車機率)
    const prob = lead.prob || 0;
    this.elements.prob.textContent = prob.toFixed(2);

    // Prob 低於閾值 (0.5)
    if (prob < 0.5 && prob > 0.05) {
      this.elements.prob.classList.add('warn-value');
      this.elements.probWarn.classList.remove('hidden');
    } else {
      this.elements.prob.classList.remove('warn-value');
      this.elements.probWarn.classList.add('hidden');
    }
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

    // 顯示 TTC
    const ttcElem = this.elements.ttc;
    if (ttc > 99) {
      ttcElem.textContent = '--';
      ttcElem.classList.remove('ttc-danger', 'ttc-warning', 'ttc-caution');
    } else {
      ttcElem.textContent = ttc.toFixed(1);

      // TTC 顏色分級
      ttcElem.classList.remove('ttc-danger', 'ttc-warning', 'ttc-caution');
      if (ttc < 3) {
        ttcElem.classList.add('ttc-danger');  // 紅色閃爍
      } else if (ttc < 6) {
        ttcElem.classList.add('ttc-warning'); // 黃色
      } else {
        ttcElem.classList.add('ttc-caution'); // 淺黃
      }
    }
  }

  // ========== 定速模擬更新 ==========

  updateCruiseSim(lead, vEgo, controlsState, aEgo) {
    const vCruise = controlsState.vCruise || 0;  // m/s
    const vCruiseKph = vCruise * 3.6;  // 轉換為 km/h
    const vEgoKph = vEgo * 3.6;

    // 顯示原始定速
    this.elements.vcruiseOriginal.textContent = vCruiseKph.toFixed(0);

    // 獲取不確定性數據
    const xStd = (lead.xStd && lead.xStd.length > 0) ? lead.xStd[0] : 0;
    const vStd = (lead.vStd && lead.vStd.length > 0) ? lead.vStd[0] : 0;
    const prob = lead.prob || 0;

    // 計算 TTC
    const dRel = (lead.x && lead.x.length > 0) ? lead.x[0] : 0;
    const vLead = (lead.v && lead.v.length > 0) ? lead.v[0] : 0;
    const vRel = vLead - vEgo;
    let ttc = 999;
    if (vRel > 0.1) {
      ttc = dRel / vRel;
    }

    // === 條件判斷 ===
    let triggers = {
      xstd: false,
      vstd: false,
      prob: false,
      ttc: false,
      accel: false
    };

    // 檢查各項條件
    if (xStd > 2.17) triggers.xstd = true;
    if (vStd > 1.08) triggers.vstd = true;
    if (prob < 0.5 && prob > 0.05) triggers.prob = true;
    if (ttc < 6 && ttc < 99) triggers.ttc = true;

    // 危險加速場景
    const accelerating = aEgo > 0.5;
    const largeSpeedGap = (vCruiseKph - vEgoKph) > 20;
    if (accelerating && largeSpeedGap && triggers.prob) {
      triggers.accel = true;
    }

    // === 風險等級判斷 ===
    let riskLevel = 'normal';  // normal, medium, high
    let suggestedVCruiseKph = vCruiseKph;

    // 高風險：xStd 和 vStd 都超標，或 TTC < 3
    const highRisk = (triggers.xstd && triggers.vstd) || (ttc < 3 && ttc < 99);

    // 中風險：任一指標觸發
    const mediumRisk = triggers.xstd || triggers.vstd || triggers.prob ||
                       (ttc < 6 && ttc < 99) || triggers.accel;

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
    this.elements.xstd.textContent = '--';
    this.elements.vstd.textContent = '--';
    this.elements.prob.textContent = '--';
    this.elements.ttc.textContent = '--';
    this.elements.drel.textContent = '--';
    this.elements.vrel.textContent = '--';
    this.elements.vcruiseOriginal.textContent = '--';
    this.elements.vcruiseSuggested.textContent = '--';
    this.elements.vcruiseReduction.textContent = '--';

    // 隱藏所有警告標記
    this.elements.xstdWarn.classList.add('hidden');
    this.elements.vstdWarn.classList.add('hidden');
    this.elements.probWarn.classList.add('hidden');

    // 隱藏所有觸發標籤
    for (const tag of Object.values(this.elements.triggerTags)) {
      if (tag) {
        tag.classList.add('hidden');
      }
    }

    // 重置狀態
    this.elements.simStatus.textContent = '等待數據';
    this.elements.simStatus.classList.remove('normal', 'warning', 'danger');
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
