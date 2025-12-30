/**
 * Canvas 渲染器
 * 處理 3D to 2D 投影和視覺化
 */

// ==================== 工具函數 ====================

/**
 * 3D 點投影到 2D 螢幕座標
 * @param {Array} point - [x, y, z] 世界座標
 * @param {Object} calibration - 相機校正參數
 * @param {Number} width - Canvas 寬度
 * @param {Number} height - Canvas 高度
 * @returns {Array} [x, y] 螢幕座標
 */
function project3DTo2D(point, calibration, width, height) {
  const [x, y, z] = point;

  // 如果沒有校正數據，使用預設投影
  if (!calibration || !calibration.extrinsicMatrix) {
    // 簡單透視投影
    const focalLength = 910;  // openpilot 預設焦距
    const screenX = (x / Math.max(z, 0.1)) * focalLength + width / 2;
    const screenY = (y / Math.max(z, 0.1)) * focalLength + height / 2;
    return [screenX, screenY];
  }

  // 使用校正矩陣
  // TODO: 完整的矩陣變換實現
  const focalLength = calibration.intrinsicMatrix ? calibration.intrinsicMatrix[0] : 910;
  const screenX = (x / Math.max(z, 0.1)) * focalLength + width / 2;
  const screenY = (y / Math.max(z, 0.1)) * focalLength + height / 2;

  return [screenX, screenY];
}

/**
 * 繪製多邊形
 */
function drawPolygon(ctx, points) {
  if (points.length < 2) return;

  ctx.beginPath();
  ctx.moveTo(points[0][0], points[0][1]);
  for (let i = 1; i < points.length; i++) {
    ctx.lineTo(points[i][0], points[i][1]);
  }
  ctx.closePath();
  ctx.fill();
}

/**
 * 繪製線條
 */
function drawLine(ctx, points) {
  if (points.length < 2) return;

  ctx.beginPath();
  ctx.moveTo(points[0][0], points[0][1]);
  for (let i = 1; i < points.length; i++) {
    ctx.lineTo(points[i][0], points[i][1]);
  }
  ctx.stroke();
}

// ==================== ModelRenderer (車道線和路徑) ====================

class ModelRenderer {
  constructor() {
    this.arMode = false;
    this.calibration = null;
  }

  setARMode(enabled) {
    this.arMode = enabled;
  }

  setCalibration(calibration) {
    this.calibration = calibration;
  }

  render(ctx, stateManager, width, height) {
    const modelV2 = stateManager.modelV2;
    if (!modelV2) return;

    // 繪製路徑
    this.drawPath(ctx, modelV2, width, height);

    // 繪製車道線
    this.drawLaneLines(ctx, modelV2, width, height);

    // 繪製前車
    this.drawLeadCar(ctx, modelV2, width, height);
  }

  drawPath(ctx, modelV2, width, height) {
    if (!modelV2.position || !modelV2.position.x || modelV2.position.x.length < 2) {
      return;
    }

    const x = modelV2.position.x;
    const y = modelV2.position.y;
    const z = modelV2.position.z;

    // 轉換為螢幕座標
    const points = [];
    for (let i = 0; i < x.length; i += 2) {  // 每隔一個點以提高性能
      const point3D = [y[i], z[i], x[i]];  // openpilot 座標系: x=前, y=左, z=上
      const point2D = project3DTo2D(point3D, this.calibration, width, height);
      if (point2D[1] > 0 && point2D[1] < height * 1.5) {  // 過濾離屏點
        points.push(point2D);
      }
    }

    if (points.length < 2) return;

    // AR 模式用發光效果
    if (this.arMode) {
      // 外層發光
      ctx.save();
      ctx.shadowBlur = 20;
      ctx.shadowColor = 'rgba(0, 255, 100, 0.6)';
      ctx.strokeStyle = 'rgba(0, 255, 100, 0.4)';
      ctx.lineWidth = 8;
      drawLine(ctx, points);
      ctx.restore();

      // 內層實線
      ctx.strokeStyle = 'rgba(0, 255, 100, 1)';
      ctx.lineWidth = 4;
      drawLine(ctx, points);
    } else {
      // 正常模式
      ctx.strokeStyle = 'rgba(13, 248, 122, 0.6)';
      ctx.lineWidth = 3;
      drawLine(ctx, points);
    }
  }

  drawLaneLines(ctx, modelV2, width, height) {
    if (!modelV2.laneLines || modelV2.laneLines.length === 0) {
      return;
    }

    const color = this.arMode ?
      'rgba(255, 255, 255, 0.9)' :  // AR 模式: 亮白色
      'rgba(255, 255, 255, 0.5)';   // 正常模式

    const lineWidth = this.arMode ? 3 : 2;

    ctx.strokeStyle = color;
    ctx.lineWidth = lineWidth;

    // 繪製每條車道線
    for (const laneLine of modelV2.laneLines) {
      if (!laneLine.x || laneLine.x.length < 2) continue;

      const points = [];
      for (let i = 0; i < laneLine.x.length; i += 2) {
        const point3D = [laneLine.y[i], laneLine.z ? laneLine.z[i] : 0, laneLine.x[i]];
        const point2D = project3DTo2D(point3D, this.calibration, width, height);
        if (point2D[1] > 0 && point2D[1] < height * 1.5) {
          points.push(point2D);
        }
      }

      if (points.length >= 2) {
        drawLine(ctx, points);
      }
    }
  }

  drawLeadCar(ctx, modelV2, width, height) {
    if (!modelV2.leads || modelV2.leads.length === 0) {
      return;
    }

    const lead = modelV2.leads[0];  // 第一輛前車
    if (!lead || lead.prob < 0.5) return;  // 機率太低不顯示

    // 前車位置
    const x = lead.x[0];  // 縱向距離
    const y = lead.y[0];  // 橫向偏移
    const z = 0;

    const point2D = project3DTo2D([y, z, x], this.calibration, width, height);

    // 繪製警告框
    const boxSize = this.arMode ? 60 : 50;
    const [cx, cy] = point2D;

    ctx.save();
    if (this.arMode) {
      ctx.shadowBlur = 15;
      ctx.shadowColor = 'rgba(255, 0, 0, 0.8)';
    }

    ctx.strokeStyle = 'rgba(255, 0, 0, 0.9)';
    ctx.lineWidth = 3;
    ctx.strokeRect(
      cx - boxSize / 2,
      cy - boxSize / 2,
      boxSize,
      boxSize
    );

    // 顯示距離
    if (x > 0) {
      ctx.fillStyle = this.arMode ? 'rgba(255, 255, 255, 1)' : 'rgba(255, 255, 255, 0.9)';
      ctx.font = this.arMode ? 'bold 18px Arial' : 'bold 16px Arial';
      ctx.textAlign = 'center';
      ctx.fillText(`${x.toFixed(0)}m`, cx, cy - boxSize / 2 - 10);
    }

    ctx.restore();
  }
}

// ==================== HudRenderer (速度、狀態) ====================

class HudRenderer {
  constructor() {
    this.arMode = false;
    this.hudMode = false;
  }

  setARMode(enabled) {
    this.arMode = enabled;
  }

  setHUDMode(enabled) {
    this.hudMode = enabled;
  }

  render(ctx, stateManager, width, height) {
    const carState = stateManager.carState;
    const controlsState = stateManager.controlsState;

    if (!carState) return;

    // 當前速度
    this.drawSpeed(ctx, carState, width, height);

    // Cruise 設定速度
    if (controlsState && controlsState.vCruise) {
      this.drawCruiseSpeed(ctx, controlsState, width, height);
    }

    // Engage 狀態
    if (controlsState && controlsState.enabled) {
      this.drawEngageStatus(ctx, width, height);
    }
  }

  drawSpeed(ctx, carState, width, height) {
    // 轉換為 km/h
    const speed = (carState.vEgo || 0) * 3.6;

    const x = width * 0.1;
    const y = height * 0.85;

    ctx.save();

    // AR 模式用發光效果
    if (this.arMode) {
      ctx.shadowBlur = 15;
      ctx.shadowColor = 'rgba(0, 255, 255, 0.8)';
    }

    // 背景
    if (!this.arMode) {
      ctx.fillStyle = 'rgba(0, 0, 0, 0.6)';
      ctx.fillRect(x - 10, y - 60, 120, 70);
    }

    // 速度數字
    ctx.fillStyle = this.arMode ? 'rgba(0, 255, 255, 1)' : 'rgba(255, 255, 255, 0.95)';
    ctx.font = this.arMode ? 'bold 48px Arial' : 'bold 44px Arial';
    ctx.textAlign = 'left';
    ctx.fillText(speed.toFixed(0), x, y);

    // 單位
    ctx.font = this.arMode ? 'bold 20px Arial' : 'bold 18px Arial';
    ctx.fillStyle = this.arMode ? 'rgba(255, 255, 255, 0.9)' : 'rgba(255, 255, 255, 0.7)';
    ctx.fillText('km/h', x + 5, y + 20);

    ctx.restore();
  }

  drawCruiseSpeed(ctx, controlsState, width, height) {
    const cruiseSpeed = controlsState.vCruise;

    const x = width * 0.9;
    const y = height * 0.85;

    ctx.save();

    if (this.arMode) {
      ctx.shadowBlur = 15;
      ctx.shadowColor = 'rgba(100, 200, 255, 0.8)';
    }

    // 背景
    if (!this.arMode) {
      ctx.fillStyle = 'rgba(0, 0, 0, 0.6)';
      ctx.fillRect(x - 110, y - 60, 120, 70);
    }

    // 速度數字
    ctx.fillStyle = this.arMode ? 'rgba(100, 200, 255, 1)' : 'rgba(100, 200, 255, 0.95)';
    ctx.font = this.arMode ? 'bold 40px Arial' : 'bold 36px Arial';
    ctx.textAlign = 'right';
    ctx.fillText(cruiseSpeed.toFixed(0), x, y);

    // 標籤
    ctx.font = this.arMode ? 'bold 16px Arial' : 'bold 14px Arial';
    ctx.fillStyle = this.arMode ? 'rgba(255, 255, 255, 0.9)' : 'rgba(255, 255, 255, 0.7)';
    ctx.fillText('CRUISE', x, y + 20);

    ctx.restore();
  }

  drawEngageStatus(ctx, width, height) {
    const x = width / 2;
    const y = height * 0.1;

    ctx.save();

    if (this.arMode) {
      ctx.shadowBlur = 20;
      ctx.shadowColor = 'rgba(0, 255, 0, 0.8)';
    }

    // 綠色指示器
    ctx.fillStyle = this.arMode ? 'rgba(0, 255, 100, 1)' : 'rgba(0, 255, 100, 0.8)';
    ctx.font = this.arMode ? 'bold 24px Arial' : 'bold 20px Arial';
    ctx.textAlign = 'center';
    ctx.fillText('✓ ENGAGED', x, y);

    ctx.restore();
  }
}

// ==================== MainRenderer (主渲染器) ====================

export class MainRenderer {
  constructor() {
    this.modelRenderer = new ModelRenderer();
    this.hudRenderer = new HudRenderer();
    this.calibration = null;
  }

  setHUDMode(enabled) {
    this.hudRenderer.setHUDMode(enabled);
  }

  setARMode(enabled) {
    this.modelRenderer.setARMode(enabled);
    this.hudRenderer.setARMode(enabled);
  }

  updateCalibration(liveCalibration) {
    if (liveCalibration && liveCalibration.extrinsicMatrix) {
      this.calibration = liveCalibration;
      this.modelRenderer.setCalibration(liveCalibration);
    }
  }

  render(ctx, stateManager, width, height) {
    // 更新校正
    if (stateManager.liveCalibration) {
      this.updateCalibration(stateManager.liveCalibration);
    }

    // 繪製車道模型
    this.modelRenderer.render(ctx, stateManager, width, height);

    // 繪製 HUD
    this.hudRenderer.render(ctx, stateManager, width, height);
  }
}
