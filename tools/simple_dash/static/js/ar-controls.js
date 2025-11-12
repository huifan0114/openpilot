/**
 * AR HUD 模式控制
 * - 拖曳移動
 * - 雙指縮放
 * - 設定儲存
 */

export class ARController {
  constructor() {
    this.container = document.getElementById('canvasContainer');
    this.controlPanel = document.getElementById('controlPanel');

    this.state = {
      arMode: false,
      hudMode: false,
      fullscreen: false,
      brightness: 100,
      scale: 100,
      position: { x: 10, y: 10 }, // 百分比
      size: { width: 80, height: 80 }, // 百分比

      // 四角座標（百分比，相對於容器）
      keystoneMode: false,
      keystone: {
        tl: { x: 0, y: 0 },       // 左上角
        tr: { x: 100, y: 0 },     // 右上角
        bl: { x: 0, y: 100 },     // 左下角
        br: { x: 100, y: 100 }    // 右下角
      }
    };

    // 觸控狀態
    this.isDragging = false;
    this.isPinching = false;
    this.lastTouchDistance = 0;
    this.touchStartPos = { x: 0, y: 0 };

    this.loadSettings();
    this.initControls();
    this.initTouch();
    this.applySettings();
  }

  // ========== 載入/儲存設定 ==========

  loadSettings() {
    try {
      const saved = localStorage.getItem('arHudSettings');
      if (saved) {
        this.state = { ...this.state, ...JSON.parse(saved) };
      }
    } catch (e) {
      console.error('Failed to load settings:', e);
    }
  }

  saveSettings() {
    try {
      localStorage.setItem('arHudSettings', JSON.stringify(this.state));
    } catch (e) {
      console.error('Failed to save settings:', e);
    }
  }

  // ========== 控制按鈕 ==========

  initControls() {
    // AR 模式切換
    document.getElementById('arModeToggle').addEventListener('click', () => {
      this.toggleARMode();
    });

    // HUD 鏡像切換
    document.getElementById('hudToggle').addEventListener('click', () => {
      this.toggleHUDMode();
    });

    // 四角調整切換
    document.getElementById('keystoneToggle').addEventListener('click', () => {
      this.toggleKeystoneMode();
    });

    // 全螢幕切換
    document.getElementById('fullscreenToggle').addEventListener('click', () => {
      this.toggleFullscreen();
    });

    // 重置位置
    document.getElementById('resetPosition').addEventListener('click', () => {
      this.resetPosition();
    });

    // 亮度滑桿
    const brightnessSlider = document.getElementById('brightnessSlider');
    brightnessSlider.value = this.state.brightness;
    brightnessSlider.addEventListener('input', (e) => {
      this.state.brightness = parseInt(e.target.value);
      document.getElementById('brightnessValue').textContent = this.state.brightness + '%';
      this.applyBrightness();
      this.saveSettings();
    });

    // 縮放滑桿
    const scaleSlider = document.getElementById('scaleSlider');
    scaleSlider.value = this.state.scale;
    scaleSlider.addEventListener('input', (e) => {
      this.state.scale = parseInt(e.target.value);
      document.getElementById('scaleValue').textContent = this.state.scale + '%';
      this.applyScale();
      this.saveSettings();
    });

    // 點擊容器外隱藏控制面板
    let hideTimer;
    document.addEventListener('click', (e) => {
      if (this.state.fullscreen && !this.controlPanel.contains(e.target)) {
        clearTimeout(hideTimer);
        this.controlPanel.classList.add('hidden');
        hideTimer = setTimeout(() => {
          this.controlPanel.classList.remove('hidden');
        }, 3000);
      }
    });
  }

  // ========== 模式切換 ==========

  toggleARMode() {
    this.state.arMode = !this.state.arMode;
    const btn = document.getElementById('arModeToggle');

    if (this.state.arMode) {
      this.container.classList.add('ar-mode');
      btn.classList.add('active');
      btn.textContent = 'Normal Mode';
    } else {
      this.container.classList.remove('ar-mode');
      btn.classList.remove('active');
      btn.textContent = 'AR Mode';
    }

    this.saveSettings();
  }

  toggleHUDMode() {
    this.state.hudMode = !this.state.hudMode;
    const btn = document.getElementById('hudToggle');

    if (this.state.hudMode) {
      this.container.classList.add('hud-mode');
      btn.classList.add('active');
      btn.textContent = 'Normal View';
    } else {
      this.container.classList.remove('hud-mode');
      btn.classList.remove('active');
      btn.textContent = 'HUD Mirror';
    }

    this.saveSettings();
  }

  toggleFullscreen() {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen().then(() => {
        this.state.fullscreen = true;
        document.body.classList.add('fullscreen');
        document.getElementById('fullscreenToggle').textContent = 'Exit Fullscreen';

        // 隱藏 Android 導航列
        if (screen.orientation && screen.orientation.lock) {
          screen.orientation.lock('landscape').catch(() => {});
        }
      }).catch((e) => {
        console.error('Fullscreen request failed:', e);
      });
    } else {
      document.exitFullscreen().then(() => {
        this.state.fullscreen = false;
        document.body.classList.remove('fullscreen');
        document.getElementById('fullscreenToggle').textContent = 'Fullscreen';
      });
    }
  }

  // ========== 觸控操作 ==========

  initTouch() {
    let startTransform = { x: 0, y: 0, scale: 1 };

    this.container.addEventListener('touchstart', (e) => {
      if (e.touches.length === 1) {
        // 單指拖曳
        this.isDragging = true;
        this.container.classList.add('dragging');
        this.container.classList.add('show-border');

        const touch = e.touches[0];
        this.touchStartPos = {
          x: touch.clientX,
          y: touch.clientY
        };

        startTransform = {
          x: this.state.position.x,
          y: this.state.position.y,
          scale: this.state.scale
        };
      } else if (e.touches.length === 2) {
        // 雙指縮放
        this.isPinching = true;
        this.isDragging = false;
        this.container.classList.add('show-border');

        const dx = e.touches[0].clientX - e.touches[1].clientX;
        const dy = e.touches[0].clientY - e.touches[1].clientY;
        this.lastTouchDistance = Math.sqrt(dx * dx + dy * dy);

        startTransform.scale = this.state.scale;
      }
    });

    this.container.addEventListener('touchmove', (e) => {
      e.preventDefault();

      if (this.isDragging && e.touches.length === 1) {
        // 拖曳移動
        const touch = e.touches[0];
        const dx = touch.clientX - this.touchStartPos.x;
        const dy = touch.clientY - this.touchStartPos.y;

        this.state.position.x = startTransform.x + (dx / window.innerWidth) * 100;
        this.state.position.y = startTransform.y + (dy / window.innerHeight) * 100;

        // 限制範圍
        this.state.position.x = Math.max(-50, Math.min(50, this.state.position.x));
        this.state.position.y = Math.max(-50, Math.min(50, this.state.position.y));

        this.applyPosition();
      } else if (this.isPinching && e.touches.length === 2) {
        // 雙指縮放
        const dx = e.touches[0].clientX - e.touches[1].clientX;
        const dy = e.touches[0].clientY - e.touches[1].clientY;
        const distance = Math.sqrt(dx * dx + dy * dy);

        const scale = (distance / this.lastTouchDistance);
        this.state.scale = Math.max(50, Math.min(200, startTransform.scale * scale));

        document.getElementById('scaleSlider').value = this.state.scale;
        document.getElementById('scaleValue').textContent = Math.round(this.state.scale) + '%';

        this.applyScale();
      }
    });

    this.container.addEventListener('touchend', (e) => {
      if (e.touches.length === 0) {
        this.isDragging = false;
        this.isPinching = false;
        this.container.classList.remove('dragging');
        setTimeout(() => {
          this.container.classList.remove('show-border');
        }, 1000);

        this.saveSettings();
      }
    });
  }

  // ========== 套用設定 ==========

  applySettings() {
    this.applyPosition();
    this.applyScale();
    this.applyBrightness();

    // 恢復四角設定
    if (this.state.keystone) {
      this.applyKeystone();
    }

    // 恢復模式
    if (this.state.arMode) {
      document.getElementById('arModeToggle').click();
    }
    if (this.state.hudMode) {
      document.getElementById('hudToggle').click();
    }
  }

  applyPosition() {
    this.container.style.left = this.state.position.x + '%';
    this.container.style.top = this.state.position.y + '%';
  }

  applyScale() {
    const scale = this.state.scale / 100;
    this.state.size.width = 80 * scale;
    this.state.size.height = 80 * scale;

    this.container.style.width = this.state.size.width + '%';
    this.container.style.height = this.state.size.height + '%';
  }

  applyBrightness() {
    const brightness = this.state.brightness / 100;
    this.container.style.setProperty('--brightness', brightness);
  }

  resetPosition() {
    this.state.position = { x: 10, y: 10 };
    this.state.size = { width: 80, height: 80 };
    this.state.scale = 100;
    this.state.brightness = 100;

    // 重置四角
    this.resetKeystone();

    document.getElementById('scaleSlider').value = 100;
    document.getElementById('scaleValue').textContent = '100%';
    document.getElementById('brightnessSlider').value = 100;
    document.getElementById('brightnessValue').textContent = '100%';

    this.applySettings();
    this.saveSettings();
  }

  // ========== 四角調整（梯形校正） ==========

  toggleKeystoneMode() {
    this.state.keystoneMode = !this.state.keystoneMode;
    const btn = document.getElementById('keystoneToggle');
    const controls = document.querySelector('.keystone-controls');

    if (this.state.keystoneMode) {
      controls.classList.remove('hidden');
      this.container.classList.add('keystone-active');
      btn.classList.add('active');
      btn.textContent = '完成調整';
      this.initKeystoneControls();
      this.updateHandlePositions();
    } else {
      controls.classList.add('hidden');
      this.container.classList.remove('keystone-active');
      btn.classList.remove('active');
      btn.textContent = '四角調整';
    }

    this.saveSettings();
  }

  initKeystoneControls() {
    const handles = document.querySelectorAll('.corner-handle');

    handles.forEach(handle => {
      let startPos = { x: 0, y: 0 };
      let startCorner = { x: 0, y: 0 };
      const corner = handle.dataset.corner;

      const onStart = (e) => {
        e.stopPropagation();  // 防止觸發容器的拖曳
        const touch = e.touches ? e.touches[0] : e;
        startPos = { x: touch.clientX, y: touch.clientY };
        startCorner = { ...this.state.keystone[corner] };
        handle.style.transition = 'none';
      };

      const onMove = (e) => {
        e.preventDefault();
        e.stopPropagation();

        const touch = e.touches ? e.touches[0] : e;
        const dx = touch.clientX - startPos.x;
        const dy = touch.clientY - startPos.y;

        // 計算新座標（百分比）
        const rect = this.container.getBoundingClientRect();
        const deltaX = (dx / rect.width) * 100;
        const deltaY = (dy / rect.height) * 100;

        this.state.keystone[corner] = {
          x: Math.max(-20, Math.min(120, startCorner.x + deltaX)),
          y: Math.max(-20, Math.min(120, startCorner.y + deltaY))
        };

        this.applyKeystone();
        this.updateHandlePositions();
      };

      const onEnd = (e) => {
        handle.style.transition = '';
        this.saveSettings();
      };

      // 同時支援觸控和滑鼠
      handle.addEventListener('touchstart', onStart);
      handle.addEventListener('touchmove', onMove);
      handle.addEventListener('touchend', onEnd);
      handle.addEventListener('mousedown', onStart);

      // 滑鼠事件需要綁定到 document
      handle._mouseMoveHandler = (e) => {
        if (e.buttons === 1) onMove(e);
      };
      handle._mouseUpHandler = onEnd;

      document.addEventListener('mousemove', handle._mouseMoveHandler);
      document.addEventListener('mouseup', handle._mouseUpHandler);
    });
  }

  applyKeystone() {
    const k = this.state.keystone;
    const canvas = document.getElementById('canvas');

    // 使用 CSS clip-path 實現四角變形
    // 這比 matrix3d 更簡單且跨瀏覽器兼容性更好
    const clipPath = `polygon(${k.tl.x}% ${k.tl.y}%, ${k.tr.x}% ${k.tr.y}%, ${k.br.x}% ${k.br.y}%, ${k.bl.x}% ${k.bl.y}%)`;

    canvas.style.clipPath = clipPath;
    canvas.style.webkitClipPath = clipPath;  // Safari 兼容
  }

  updateHandlePositions() {
    const k = this.state.keystone;

    const updateHandle = (corner, selector) => {
      const handle = document.querySelector(selector);
      if (handle) {
        handle.style.left = `calc(${k[corner].x}% - 25px)`;
        handle.style.top = `calc(${k[corner].y}% - 25px)`;
      }
    };

    updateHandle('tl', '.corner-handle.top-left');
    updateHandle('tr', '.corner-handle.top-right');
    updateHandle('bl', '.corner-handle.bottom-left');
    updateHandle('br', '.corner-handle.bottom-right');
  }

  resetKeystone() {
    this.state.keystone = {
      tl: { x: 0, y: 0 },
      tr: { x: 100, y: 0 },
      bl: { x: 0, y: 100 },
      br: { x: 100, y: 100 }
    };
    this.applyKeystone();
    this.updateHandlePositions();
  }

  // ========== 取得 HUD 模式狀態 (給渲染器使用) ==========

  isHUDMode() {
    return this.state.hudMode;
  }

  isARMode() {
    return this.state.arMode;
  }
}
