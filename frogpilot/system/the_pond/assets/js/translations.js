// The Pond 繁體中文翻譯
export const translations = {
  // 通用
  "Loading...": "載入中...",
  "Failed to load": "載入失敗",
  "No data available": "沒有可用的資料",
  "Confirm": "確認",
  "Cancel": "取消",
  "Delete": "刪除",
  "Save": "儲存",
  "Yes": "是",
  "No": "否",
  "Unknown": "未知",

  // 側邊欄
  "Home": "首頁",
  "Navigation": "導航",
  "Manage Keys": "管理金鑰",
  "Set Destination": "設定目的地",
  "Recordings": "錄影",
  "Dashcam Routes": "行車記錄",
  "Screen Recordings": "螢幕錄影",
  "Tailscale": "Tailscale",
  "Tools": "工具",
  "Download Speed Limits": "下載速限",
  "Error Logs": "錯誤日誌",
  "Lock/Unlock Doors": "鎖定/解鎖車門",
  "Theme Maker": "主題製作",
  "Tmux Log": "Tmux 日誌",
  "Toggles": "功能設定",
  "Toyota Security Keys": "Toyota 安全金鑰",

  // 首頁
  "All Time": "總計",
  "Past Week": "過去一週",
  "FrogPilot": "FrogPilot",
  "drives": "次行程",
  "hours": "小時",
  "miles": "英里",
  "kilometers": "公里",
  "used of": "已使用",
  "Disk Usage": "磁碟使用率",
  "Firehose Segments": "Firehose 區段",
  "segments in training data": "個區段在訓練資料中",
  "Software Info": "軟體訊息",
  "Branch Name": "分支名稱",
  "Build": "編譯環境",
  "Commit Hash": "提交雜湊",
  "Fork Maintainer": "分支維護者",
  "Update Available": "有可用更新",
  "Version Date": "版本日期",

  // Navigation Keys
  "Navigation Provider": "導航供應商",
  "AMap Keys": "高德地圖金鑰",
  "Google Maps Key": "Google 地圖金鑰",
  "Mapbox Keys": "Mapbox 金鑰",
  "Public": "公開",
  "Secret": "私密",
  "API": "API",
  "Key": "金鑰",
  "Confirm Delete": "確認刪除",
  "Are you sure you want to delete": "您確定要刪除",
  "Yes, Delete": "是的，刪除",
  "Failed to load keys": "載入金鑰失敗",
  "Failed to save": "儲存失敗",
  "Failed to delete": "刪除失敗",
  "Switched to": "已切換至",
  "Google Maps": "Google 地圖",
  "Mapbox": "Mapbox",

  // Error Logs
  "Select All": "全選",
  "Deselect All": "取消全選",
  "Upload": "上傳",
  "Delete Selected": "刪除已選擇",

  // Speed Limits
  "Search": "搜尋",
  "Download": "下載",
  "Downloaded": "已下載",

  // Toggles
  "Reset": "重置",
  "Apply": "套用",

  // Theme Maker
  "Preview": "預覽",
  "Export": "匯出",
  "Import": "匯入",

  // Dashcam Routes
  "Date": "日期",
  "Duration": "時長",
  "Distance": "距離",

  // Screen Recordings
  "Name": "名稱",
  "Size": "大小",
  "Actions": "操作",

  // Doors
  "Lock": "鎖定",
  "Unlock": "解鎖",
  "Status": "狀態",

  // TSK Manager
  "Serial Number": "序號",
  "Generate": "生成",

  // 單位
  "GB": "GB",
  "MB": "MB",
  "KB": "KB",
  "seconds": "秒",
  "minutes": "分鐘",
  "hours": "小時",
};

// 翻譯函數
export function t(key) {
  return translations[key] || key;
}

// 批量翻譯 HTML 內容
export function translateHTML(html) {
  let translated = html;
  for (const [en, zh] of Object.entries(translations)) {
    translated = translated.replace(new RegExp(en, 'g'), zh);
  }
  return translated;
}
