// Import the functions you need from the SDKs you need
import { initializeApp } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-app.js";
import { getDatabase, ref, onValue } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";
import { getAuth, signInAnonymously } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-auth.js";

// Import modular pages
import { initDashboard } from "./modules/dashboard.js?v=1.0.2";
import { initRFID } from "./modules/rfid.js?v=1.0.2";
import { initLogs } from "./modules/logs.js?v=1.0.2";
import { initConfig } from "./modules/config.js?v=1.0.2";

// Firebase config được inject bởi CI (xem firebase-config.example.js để chạy local)
import { firebaseConfig } from "./firebase-config.js";

// Initialize Firebase
const app = initializeApp(firebaseConfig);
export const db = getDatabase(app);

// Đăng nhập ẩn danh để đáp ứng Firebase Security Rules (auth != null)
const auth = getAuth(app);
signInAnonymously(auth).catch((err) => console.error("Firebase auth error:", err));

// Trạng thái toàn cục để đồng bộ logic hiển thị cảnh báo
export const state = {
    sensors: { rain: false, motion: false, temperature: null, humidity: null },
    devices: {
        indoor_light: { status: false, schedule: { enabled: false } },
        outdoor_light: { status: false, mode: "auto", schedule: { enabled: false } },
        door: { status: "closed" },
        roof: { status: "open", mode: "auto" }
    }
};

// Keep track of database connection
const connectedRef = ref(db, ".info/connected");
onValue(connectedRef, (snap) => {
    const statusDot = document.querySelector("#db-status .status-dot");
    const statusText = document.querySelector("#db-status .status-text");
    if (!statusDot || !statusText) return;
    if (snap.val() === true) {
        statusDot.className = "status-dot online";
        statusText.innerText = "Connected";
    } else {
        statusDot.className = "status-dot offline";
        statusText.innerText = "Disconnected";
    }
});

// ==========================================
// Toast Notification Helper
// ==========================================
export function showToast(message, type = 'success', duration = 3500) {
    const container = document.getElementById("toast-container");
    if (!container) return;

    const toast = document.createElement("div");
    toast.className = `toast ${type}`;

    let iconClass = "fa-solid fa-circle-check";
    if (type === "error") iconClass = "fa-solid fa-circle-xmark";
    else if (type === "info") iconClass = "fa-solid fa-circle-info";

    toast.innerHTML = `
        <div class="toast-icon"><i class="${iconClass}"></i></div>
        <div class="toast-content">${message}</div>
        <button class="toast-close"><i class="fa-solid fa-xmark"></i></button>
    `;

    container.appendChild(toast);

    // Trigger transition reflow
    setTimeout(() => {
        toast.classList.add("show");
    }, 10);

    const closeBtn = toast.querySelector(".toast-close");
    if (closeBtn) {
        closeBtn.addEventListener("click", () => {
            removeToast(toast);
        });
    }

    const timeoutId = setTimeout(() => {
        removeToast(toast);
    }, duration);

    function removeToast(el) {
        el.classList.remove("show");
        el.addEventListener("transitionend", () => {
            el.remove();
        });
    }
}

// ==========================================
// Custom Confirm Modal Helper
// ==========================================
export function showConfirm(message, onConfirm) {
    if (document.getElementById("custom-confirm-modal")) return;

    const modal = document.createElement("div");
    modal.id = "custom-confirm-modal";
    modal.className = "confirm-modal-backdrop";

    modal.innerHTML = `
        <div class="confirm-modal-box">
            <div class="confirm-modal-header">
                <i class="fa-solid fa-circle-question confirm-modal-icon"></i>
                <h3>Xác nhận hành động</h3>
            </div>
            <div class="confirm-modal-body">
                <p>${message}</p>
            </div>
            <div class="confirm-modal-actions">
                <button class="confirm-btn-cancel" id="confirm-btn-cancel">Hủy</button>
                <button class="confirm-btn-ok" id="confirm-btn-ok">Xác nhận</button>
            </div>
        </div>
    `;

    document.body.appendChild(modal);

    setTimeout(() => {
        modal.classList.add("show");
    }, 10);

    const closeConfirm = () => {
        modal.classList.remove("show");
        modal.addEventListener("transitionend", () => {
            modal.remove();
        });
    };

    modal.querySelector("#confirm-btn-cancel").addEventListener("click", closeConfirm);
    modal.querySelector("#confirm-btn-ok").addEventListener("click", () => {
        closeConfirm();
        if (onConfirm) onConfirm();
    });

    modal.addEventListener("click", (e) => {
        if (e.target === modal) closeConfirm();
    });
}

// ==========================================
// 1. Giao diện, Điều hướng Tab & Sáng/Tối
// ==========================================
const menuItems = document.querySelectorAll(".menu-item");
const mobileNavItems = document.querySelectorAll(".mobile-nav-item");
const tabContents = document.querySelectorAll(".tab-content");
const pageTitle = document.getElementById("page-title");
const themeToggleBtn = document.getElementById("btn-theme-toggle");

const tabTitles = {
    dashboard: "Hệ thống Điều khiển",
    rfid: "Quản lý thẻ RFID",
    logs: "Nhật ký Hoạt động",
    config: "Cấu hình Hệ thống"
};

// Hàm cập nhật UI tab (không thay đổi URL)
function applyTab(tabName) {
    menuItems.forEach(i => {
        if (i.getAttribute("data-tab") === tabName) i.classList.add("active");
        else i.classList.remove("active");
    });
    mobileNavItems.forEach(i => {
        if (i.getAttribute("data-tab") === tabName) i.classList.add("active");
        else i.classList.remove("active");
    });
    tabContents.forEach(tab => {
        if (tab.id === `tab-${tabName}`) tab.classList.add("active");
        else tab.classList.remove("active");
    });
    if (pageTitle) pageTitle.innerText = tabTitles[tabName] || tabTitles.dashboard;
}

// Hàm chuyển đổi tab: cập nhật UI + URL hash
function switchTab(tabName) {
    if (!tabTitles[tabName]) tabName = "dashboard";
    applyTab(tabName);
    if (location.hash !== "#" + tabName) {
        history.pushState({ tab: tabName }, "", "#" + tabName);
    }
}

// Xử lý nút Back/Forward của trình duyệt
window.addEventListener("popstate", (e) => {
    const tab = e.state?.tab || location.hash.slice(1) || "dashboard";
    applyTab(tabTitles[tab] ? tab : "dashboard");
});

// Gắn sự kiện cho Sidebar
menuItems.forEach(item => {
    item.addEventListener("click", (e) => {
        e.preventDefault();
        switchTab(item.getAttribute("data-tab"));
    });
});

// Gắn sự kiện cho Mobile Nav
mobileNavItems.forEach(item => {
    item.addEventListener("click", (e) => {
        e.preventDefault();
        switchTab(item.getAttribute("data-tab"));
    });
});

// --- Quản lý Chế độ Sáng / Tối ---
function applyTheme(theme) {
    if (!themeToggleBtn) return;
    const icon = themeToggleBtn.querySelector("i");
    if (theme === "light") {
        document.body.classList.add("light-mode");
        if (icon) icon.className = "fa-solid fa-sun";
    } else {
        document.body.classList.remove("light-mode");
        if (icon) icon.className = "fa-solid fa-moon";
    }
}

// Đọc theme đã lưu (Mặc định là light nếu chưa được thiết lập)
const savedTheme = localStorage.getItem("theme") || "light";
applyTheme(savedTheme);

if (themeToggleBtn) {
    themeToggleBtn.addEventListener("click", () => {
        const currentTheme = document.body.classList.contains("light-mode") ? "light" : "dark";
        const newTheme = currentTheme === "light" ? "dark" : "light";
        localStorage.setItem("theme", newTheme);
        applyTheme(newTheme);
    });
}

// Hiển thị giờ thời gian thực
function updateTime() {
    const liveTimeEl = document.getElementById("live-time");
    if (!liveTimeEl) return;
    const options = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric', hour: '2-digit', minute: '2-digit', second: '2-digit' };
    const date = new Date();
    liveTimeEl.innerText = date.toLocaleString('vi-VN', options);
}
setInterval(updateTime, 1000);
updateTime();

// ==========================================
// Khởi tạo các module trang
// ==========================================
initDashboard();
initRFID();
initLogs();
initConfig();

// Khởi tạo tab từ URL hash khi tải trang (giữ nguyên tab khi reload)
const initHash = location.hash.slice(1);
switchTab(tabTitles[initHash] ? initHash : "dashboard");
