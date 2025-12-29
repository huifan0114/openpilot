import { html, reactive } from "https://esm.sh/@arrow-js/core";

function DiskUsage(disk) {
  const used = parseFloat(disk.usedPercentage) || 0;
  const rightRadius = used >= 100 ? "0" : "var(--border-radius-md)";

  return html`
    <div class="disk">
      <p>已使用 ${disk.used} / 總共 ${disk.size}</p>
      <div class="progress">
        <div
          class="bar"
          style="
            border-bottom-right-radius: ${rightRadius};
            border-top-right-radius: ${rightRadius};
            width: ${100 - used}%;
          "
        ></div>
      </div>
    </div>
  `;
}

function DriveStat(title, stats = {}, defaultUnit) {
  const format = (n) =>
    n?.toLocaleString("en-US", {
      minimumFractionDigits: 0,
      maximumFractionDigits: 0,
    }) ?? "0";

  return html`
    <div class="drivingStat">
      <h2>${title}</h2>
      <div><p>${format(stats.drives)}</p><p>次行程</p></div>
      <div><p>${format(stats.distance)}</p><p>${stats.unit ?? defaultUnit}</p></div>
      <div><p>${format(stats.hours)}</p><p>小時</p></div>
    </div>
  `;
}

function renderSoftwareInfo(info = {}) {
  const fields = [
    ["分支名稱", info.branchName],
    ["編譯環境", info.buildEnvironment],
    ["提交雜湊", info.commitHash],
    ["分支維護者", info.forkMaintainer],
    ["有可用更新", info.updateAvailable],
    ["版本日期", info.versionDate],
  ];

  return fields.map(
    ([label, value]) =>
      html`<p><strong>${label}:</strong> ${value ?? "未知"}</p>`
  );
}

function renderDiskUsageSection({ diskError, diskUsage }) {
  if (diskError) {
    return html`<p>${diskError.join("<br>")}</p>`;
  }
  if (diskUsage?.length) {
    return diskUsage.map(DiskUsage);
  }
  return DiskUsage({ size: "0 GB", used: "0 GB", usedPercentage: "0" });
}

export function Home() {
  const state = reactive({
    data: null,
    unit: "miles",
    isLoading: true,
    error: null,
  });

  async function initialize() {
    try {
      const [statsResponse, unitResponse] = await Promise.all([
        fetch("/api/stats"),
        fetch("/api/params?key=IsMetric"),
      ]);

      if (!statsResponse.ok) throw new Error(`API error: ${statsResponse.statusText}`);
      if (!unitResponse.ok) throw new Error(`API error: ${unitResponse.statusText}`);

      const statsJson = await statsResponse.json();
      const isMetricText = (await unitResponse.text()).trim();
      const isMetric = isMetricText === "1";

      state.data = statsJson;
      state.unit = isMetric ? "公里" : "英里";
      localStorage.setItem("isMetric", isMetricText);
    } catch (err) {
      console.error("Failed to initialize component:", err);
      state.error = err.message;
    } finally {
      state.isLoading = false;
    }
  }

  initialize();

  return html`
    <div>
      ${() => {
        if (state.isLoading) {
          return html`<p>載入中...</p>`;
        }

        if (state.error) {
          return html`<p class="error">載入資料失敗：${state.error}</p>`;
        }

        if (state.data) {
          const { driveStats, firehoseStats, softwareInfo } = state.data;
          return html`
            <h1>The Pond</h1>

            <div class="drivingStats">
              ${DriveStat("總計", driveStats?.all, state.unit)}
              ${DriveStat("過去一週", driveStats?.week, state.unit)}
              ${DriveStat("FrogPilot", driveStats?.frogpilot, state.unit)}
            </div>

            <h2>磁碟使用率</h2>
            <div class="diskUsage">
              ${renderDiskUsageSection(state.data)}
            </div>

            <h2>Firehose 區段</h2>
            <div class="firehoseStats">
              <p>
                <strong>${(firehoseStats?.segments ?? 0).toLocaleString("en-US")}</strong>
                個區段在訓練資料中。
              </p>
            </div>

            <h2>軟體訊息</h2>
            <div class="softwareInfo">
              <div class="softwareGrid">${renderSoftwareInfo(softwareInfo)}</div>
            </div>
          `;
        }

        return html`<p>沒有可用的資料。</p>`;
      }}
    </div>
  `;
}
