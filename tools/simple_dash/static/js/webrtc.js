/**
 * WebRTC 連接管理
 * 處理視訊串流和 cereal 訊息接收
 */

export let pc = null;
export let dc = null;

/**
 * 發送 SDP offer 到 webrtcd（模仿 DASHY，直接連接）
 */
function offerRtcRequest(sdp, type) {
  // 構建 webrtcd 的請求格式
  const body = {
    sdp: sdp,
    cameras: [],  // Data Channel only
    bridge_services_in: [],  // 不需要測試聲音
    bridge_services_out: [
      "carState",               // 基本車輛狀態 (vEgo, aEgo, vCruise 等)
      "controlsState",          // 控制狀態 (定速設定值 vCruise)
      "frogpilotPlan",          // FrogPilot 路徑規劃 (速限, 彎道控制, CEM 等)
      "radarState",             // 雷達狀態 (前車資訊)
      "carOutput"               // 車輛輸出 (actuatorsOutput.accel 等)
    ]
  };

  // 直接連接 webrtcd（模仿 DASHY 架構）
  const host = window.location.hostname || 'localhost';
  const url = `http://${host}:5001/stream`;

  console.log(`[DEBUG] Connecting directly to webrtcd: ${url}`);

  return fetch(url, {
    body: JSON.stringify(body),
    headers: { 'Content-Type': 'application/json' },
    method: 'POST'
  });
}

/**
 * 建立 PeerConnection
 */
function createPeerConnection() {
  const config = {
    sdpSemantics: 'unified-plan'
  };

  pc = new RTCPeerConnection(config);

  // Data Channel only - 不處理視訊/音訊軌道
  // 因為 server.py 設定 cameras=[]，不會收到任何媒體軌道
  /*
  pc.addEventListener('track', function(evt) {
    console.log(`Adding track: ${evt.track.kind}`);
    if (evt.track.kind === 'video') {
      document.getElementById('video').srcObject = evt.streams[0];
    } else if (evt.track.kind === 'audio') {
      document.getElementById('audio').srcObject = evt.streams[0];
    }
  });
  */

  return pc;
}

/**
 * SDP 協商
 */
function negotiate() {
  // Data Channel only - 不接收音訊/視訊
  return pc.createOffer({ offerToReceiveAudio: false, offerToReceiveVideo: false })
    .then(function(offer) {
      return pc.setLocalDescription(offer);
    })
    .then(function() {
      // 等待 ICE 候選收集完成
      return new Promise(function(resolve) {
        if (pc.iceGatheringState === 'complete') {
          resolve();
        } else {
          function checkState() {
            if (pc.iceGatheringState === 'complete') {
              pc.removeEventListener('icegatheringstatechange', checkState);
              resolve();
            }
          }
          pc.addEventListener('icegatheringstatechange', checkState);
        }
      });
    })
    .then(function() {
      const offer = pc.localDescription;
      return offerRtcRequest(offer.sdp, offer.type);
    })
    .then(function(response) {
      console.log(`[DEBUG] Offer response status: ${response.status}`);

      // 檢查是否為錯誤回應
      if (!response.ok) {
        return response.json().then(errorData => {
          console.error('[ERROR] Server returned error:');
          console.error('[ERROR] Status:', response.status);
          console.error('[ERROR] Error type:', errorData.error_type);
          console.error('[ERROR] Error message:', errorData.error);
          console.error('[ERROR] Traceback:');
          console.error(errorData.traceback);
          throw new Error(`Server error: ${errorData.error}`);
        });
      }

      return response.json();
    })
    .then(function(answer) {
      console.log('[DEBUG] Received answer from server');
      console.log('[DEBUG] Answer keys:', Object.keys(answer));
      console.log('[DEBUG] SDP length:', answer.sdp ? answer.sdp.length : 0);
      console.log('[DEBUG] Type:', answer.type);

      // 驗證 answer 格式
      if (!answer.sdp || !answer.type) {
        console.error('[ERROR] Invalid answer format:', answer);
        throw new Error(`Invalid answer: missing ${!answer.sdp ? 'sdp' : 'type'} field`);
      }

      console.log('[DEBUG] Setting remote description...');
      return pc.setRemoteDescription(answer);
    })
    .catch(function(e) {
      console.error('[ERROR] Negotiation failed:', e);
      console.error('[ERROR] Error name:', e.name);
      console.error('[ERROR] Error message:', e.message);
      if (e.stack) {
        console.error('[ERROR] Stack trace:', e.stack);
      }
      throw e;
    });
}

/**
 * 啟動 WebRTC 連接
 * @param {Function} onMessage - 接收 cereal 訊息的回調函數
 * @returns {Promise}
 */
export async function start(onMessage) {
  console.log('Starting WebRTC connection...');

  // 更新狀態
  updateStatus('連接中...', false);

  // 建立 PeerConnection
  pc = createPeerConnection();

  // 建立 Data Channel (接收 cereal 訊息)
  const parameters = { ordered: true };
  dc = pc.createDataChannel('data', parameters);

  dc.onopen = function() {
    console.log('Data channel opened');
    updateStatus('已連接', true);
  };

  dc.onclose = function() {
    console.log('Data channel closed');
    updateStatus('已斷開', false);
  };

  dc.onerror = function(error) {
    console.error('Data channel error:', error);
    updateStatus('連接錯誤', false);
  };

  // 處理接收到的 cereal 訊息
  const textDecoder = new TextDecoder();
  dc.onmessage = function(evt) {
    try {
      const text = textDecoder.decode(evt.data);
      const msg = JSON.parse(text);

      // 回調處理訊息
      if (onMessage && msg.type && msg.data) {
        onMessage(msg.type, msg.data);
      }
    } catch (e) {
      console.error('Failed to parse message:', e);
    }
  };

  // 開始協商
  try {
    await negotiate();
    console.log('WebRTC connection established');
  } catch (error) {
    console.error('Failed to start WebRTC:', error);
    updateStatus('連接失敗', false);
    throw error;
  }
}

/**
 * 停止 WebRTC 連接
 */
export function stop() {
  console.log('Stopping WebRTC connection...');

  if (dc) {
    dc.close();
    dc = null;
  }

  if (pc) {
    if (pc.getTransceivers) {
      pc.getTransceivers().forEach(function(transceiver) {
        if (transceiver.stop) {
          transceiver.stop();
        }
      });
    }

    pc.getSenders().forEach(function(sender) {
      if (sender.track) {
        sender.track.stop();
      }
    });

    setTimeout(function() {
      pc.close();
      pc = null;
    }, 500);
  }

  updateStatus('已斷開', false);
}

/**
 * 更新連接狀態顯示
 */
function updateStatus(text, connected) {
  const statusText = document.getElementById('statusText');
  const statusDot = document.querySelector('.status-dot');

  if (statusText) {
    statusText.textContent = text;
  }

  if (statusDot) {
    if (connected) {
      statusDot.classList.add('connected');
    } else {
      statusDot.classList.remove('connected');
    }
  }
}

/**
 * 檢查連接狀態
 */
export function isConnected() {
  return pc && pc.connectionState === 'connected' && dc && dc.readyState === 'open';
}
