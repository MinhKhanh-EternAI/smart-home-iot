import { db } from "../app.js";
import { ref, onValue, query, limitToLast } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

let allEventLogs = [];
const logIcons = {
    system: "fa-solid fa-server",
    sensor: "fa-solid fa-droplet",
    automation: "fa-solid fa-robot",
    schedule: "fa-solid fa-clock",
    security: "fa-solid fa-shield-halved"
};

function escapeHTML(str) {
    if (!str) return "";
    return str.toString()
              .replace(/&/g, "&amp;")
              .replace(/</g, "&lt;")
              .replace(/>/g, "&gt;")
              .replace(/"/g, "&quot;")
              .replace(/'/g, "&#039;");
}

function renderEventLogs(filter) {
    const container = document.getElementById("event-logs-feed");
    if (!container) return;
    
    container.innerHTML = "";
    const filteredLogs = filter === "all" ? allEventLogs : allEventLogs.filter(log => log.type === filter);
    
    if (filteredLogs.length === 0) {
        container.innerHTML = `<p class="text-center text-muted">Không có nhật ký nào phù hợp.</p>`;
        return;
    }

    filteredLogs.forEach(log => {
        const date = new Date(log.timestamp);
        const timeStr = date.toLocaleTimeString('vi-VN') + " - " + date.toLocaleDateString('vi-VN');
        const iconClass = logIcons[log.type] || "fa-solid fa-circle-info";
        
        const row = document.createElement("div");
        row.className = `event-log-row ${escapeHTML(log.type)}`;
        row.innerHTML = `
            <div class="event-log-icon">
                <i class="${escapeHTML(iconClass)}"></i>
            </div>
            <div class="event-log-body">
                <span class="event-log-text">${escapeHTML(log.message)}</span>
                <span class="event-log-time">${escapeHTML(timeStr)}</span>
            </div>
        `;
        container.appendChild(row);
    });
}

export function initLogs() {
    const feedContainer = document.getElementById("event-logs-feed");
    if (!feedContainer) return;

    const eventLogsQuery = query(ref(db, "logs/event_logs"), limitToLast(40));
    onValue(eventLogsQuery, (snapshot) => {
        const logs = snapshot.val();
        allEventLogs = [];
        
        if (logs) {
            Object.keys(logs).forEach(key => {
                allEventLogs.push({
                    id: key,
                    ...logs[key]
                });
            });
            // Sắp xếp mới nhất lên đầu
            allEventLogs.sort((a, b) => b.timestamp - a.timestamp);
        }
        
        const activeFilterBtn = document.querySelector(".filter-btn.active");
        const currentFilter = activeFilterBtn ? activeFilterBtn.getAttribute("data-filter") : "all";
        renderEventLogs(currentFilter);
    });

    // Bộ lọc log sự kiện
    const filterBtns = document.querySelectorAll(".filter-btn");
    filterBtns.forEach(btn => {
        btn.addEventListener("click", () => {
            filterBtns.forEach(b => b.classList.remove("active"));
            btn.classList.add("active");
            
            const filter = btn.getAttribute("data-filter");
            renderEventLogs(filter);
        });
    });
}
