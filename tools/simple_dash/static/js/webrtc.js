/**
 * WebRTC 連接管理
 * 處理視訊串流和 cereal 訊息接收
 */

export let pc = null;
export let dc = null;

/**
 * 發送 SDP offer 到後端
 */
function offerRtcRequest(sdp, type) {
  return fetch('/offer', {
    body: JSON.stringify({ sdp: sdp, type: type }),
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
      return response.json();
    })
    .then(function(answer) {
      return pc.setRemoteDescription(answer);
    })
    .catch(function(e) {
      console.error('Negotiation failed:', e);
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
