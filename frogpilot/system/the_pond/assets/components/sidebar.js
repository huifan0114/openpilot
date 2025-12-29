import { html, reactive } from "https://esm.sh/@arrow-js/core";
import { Link } from "/assets/components/router.js";
import { upperFirst, hideSidebar } from "/assets/js/utils.js";

const MenuItems = {
  home: [
    { name: "首頁", link: "/", icon: "bi-house-fill" },
  ],
  navigation: [
    { name: "管理金鑰", link: "/manage_navigation_keys", icon: "bi-key-fill" },
    { name: "設定目的地", link: "/set_navigation_destination", icon: "bi-globe-americas" },
  ],
  recordings: [
    { name: "行車記錄", link: "/dashcam_routes", icon: "bi-camera-reels" },
    { name: "螢幕錄影", link: "/screen_recordings", icon: "bi-record-circle" },
  ],
  tailscale: [
    { name: "Tailscale", link: "/manage_tailscale", icon: "bi-wifi" },
  ],
  tools: [
    { name: "下載速限", link: "/download_speed_limits", icon: "bi-download" },
    { name: "錯誤日誌", link: "/manage_error_logs", icon: "bi-exclamation-triangle" },
    { name: "鎖定/解鎖車門", link: "/lock_or_unlock_doors", icon: "bi-door-closed" },
    { name: "主題製作", link: "/theme_maker", icon: "bi-palette-fill" },
    { name: "Tmux 日誌", link: "/manage_tmux", icon: "bi-terminal" },
    { name: "功能設定", link: "/manage_toggles", icon: "bi-toggle-on" },
    { name: "Toyota 安全金鑰", link: "/tsk_manager", icon: "bi-key-fill" },
  ],
};

const state = reactive({
  doorsVisible: false,
  isDoorsFetched: false,
  isTSKFetched: false,
  tskVisible: false,

  activeRoute: ""
});

export function Sidebar() {
  const currentPath = window.location.pathname;
  const activeItem = Object.values(MenuItems).flat().find(item => item.link === currentPath);
  state.activeRoute = activeItem?.name ?? "";

  if (!state.isDoorsFetched) {
    state.isDoorsFetched = true;
    (async () => {
      try {
        const response = await fetch("/api/doors_available");
        const data = await response.json();
        state.doorsVisible = data.result;
      } catch (e) {
        console.error("Failed to fetch door availability:", e);
      }
    })();
  }

  if (!state.isTSKFetched) {
    state.isTSKFetched = true;
    (async () => {
      try {
        const response = await fetch("/api/tsk_available");
        const data = await response.json();
        state.tskVisible = data.result;
      } catch (e) {
        console.error("Failed to fetch TSK availability:", e);
      }
    })();
  }

  function navigate(link) {
    state.activeRoute = link.name;
    window.scrollTo(0, 0);
    hideSidebar();

    document.querySelectorAll('.sidebar li').forEach(el => {
      el.classList.remove('active');
    });

    const linkElement = document.querySelector(`.sidebar li a[href="${link.link}"]`);
    if (linkElement) {
      linkElement.parentElement.classList.add('active');
    }
  }

  return html`
    <div id="sidebarUnderlay" class="hidden"></div>
    <div id="sidebar" class="sidebar">
      <div>
        <div class="title">
          <img class="logo" src="/assets/images/main_logo.png" alt="FrogPilot logo" />
          <div class="title_text sidebar_header">
            <p>The Pond</p>
            <a href="https://github.com/Aidenir">by&nbsp;Aidenir</a>
          </div>
        </div>
        <hr />
        ${() => Object.entries(MenuItems).map(([section, links]) => html`
          <div class="sidebar_widget">
            <ul class="menu_section">
              <li>
                <span class="section-title">${upperFirst(section)}</span>
                <ul id="${section}">
                  ${links.map(link => {
                    if (link.name === "鎖定/解鎖車門" && !state.doorsVisible) {
                      return "";
                    }

                    if (link.name === "Toyota 安全金鑰" && !state.tskVisible) {
                      return "";
                    }

                    const isActive = state.activeRoute === link.name;
                    const classList = [isActive && "active"].filter(Boolean).join(" ");

                    const content = html`
                      <div class="menu-item-link">
                        <i class="bi ${link.icon}"></i>
                        <span>${upperFirst(link.name)}</span>
                      </div>
                    `;

                    return html`
                      <li class="${classList}">
                        ${Link(link.link, content, () => navigate(link))}
                      </li>
                    `;
                  })}
                </ul>
              </li>
            </ul>
          </div>
        `)}
      </div>
    </div>`;
}

function setupMenuButton() {
  const button = document.getElementById("menu_button");
  const sidebar = document.getElementById("sidebar");
  const underlay = document.getElementById("sidebarUnderlay");

  button.addEventListener("click", () => {
    sidebar.classList.toggle("visible");
    underlay.classList.toggle("hidden");
  });

  underlay.addEventListener("click", hideSidebar);
}

document.addEventListener("DOMContentLoaded", setupMenuButton, false);
