import { db, showToast, showConfirm } from "../app.js";
import { ref, onValue, set, remove, query, limitToLast } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

function escapeHTML(str) {
    if (!str) return "";
    return str.toString()
              .replace(/&/g, "&amp;")
              .replace(/</g, "&lt;")
              .replace(/>/g, "&gt;")
              .replace(/"/g, "&quot;")
              .replace(/'/g, "&#039;");
}

export function initRFID() {
    // Load danh sách thẻ RFID
    onValue(ref(db, "rfid/cards"), (snapshot) => {
        const tbody = document.getElementById("rfid-cards-table");
        if (!tbody) return;
        
        tbody.innerHTML = "";
        const cards = snapshot.val();
        if (!cards) {
            tbody.innerHTML = `<tr><td colspan="4" class="text-center">Chưa có thẻ nào được đăng ký.</td></tr>`;
            return;
        }

        Object.keys(cards).forEach(uid => {
            const card = cards[uid];
            const tr = document.createElement("tr");
            
            tr.innerHTML = `
                <td><code>${escapeHTML(uid)}</code></td>
                <td>${escapeHTML(card.name)}</td>
                <td>
                    <label class="switch-sm">
                        <input type="checkbox" class="rfid-active-toggle" data-uid="${uid}" ${card.active ? "checked" : ""}>
                        <span class="slider-sm"></span>
                    </label>
                </td>
                <td>
                    <button class="danger-btn rfid-delete-btn" data-uid="${uid}">Xóa</button>
                </td>
            `;
            
            // Xử lý bật/tắt hoạt động thẻ
            tr.querySelector(".rfid-active-toggle").addEventListener("change", (e) => {
                const isChecked = e.target.checked;
                set(ref(db, `rfid/cards/${uid}/active`), isChecked).then(() => {
                    const statusText = isChecked ? "Đã kích hoạt" : "Đã khóa";
                    showToast(`${statusText} thẻ của ${card.name} thành công!`, "success");
                }).catch(err => {
                    showToast("Lỗi cập nhật trạng thái: " + err.message, "error");
                });
            });

            // Xử lý xóa thẻ
            tr.querySelector(".rfid-delete-btn").addEventListener("click", () => {
                showConfirm(`Bạn có chắc chắn muốn xóa thẻ của ${card.name} (UID: ${uid})?`, () => {
                    remove(ref(db, `rfid/cards/${uid}`)).then(() => {
                        showToast(`Đã xóa thẻ của ${card.name} thành công!`, "success");
                    }).catch(err => {
                        showToast("Lỗi xóa thẻ: " + err.message, "error");
                    });
                });
            });

            tbody.appendChild(tr);
        });
    });

    // Đăng ký thẻ mới
    const addCardForm = document.getElementById("add-card-form");
    if (addCardForm) {
        addCardForm.addEventListener("submit", (e) => {
            e.preventDefault();
            let uid = document.getElementById("new-card-uid").value.trim().toUpperCase();
            let name = document.getElementById("new-card-name").value.trim();

            if (!uid || !name) return;

            // Bỏ khoảng cách hoặc các ký tự lạ nếu có
            uid = uid.replace(/[^A-F0-9]/g, "");

            set(ref(db, `rfid/cards/${uid}`), {
                name: name,
                active: true,
                created_at: Date.now()
            }).then(() => {
                showToast("Đăng ký thẻ thành công!", "success");
                addCardForm.reset();
            }).catch(err => {
                showToast("Lỗi đăng ký thẻ: " + err.message, "error");
            });
        });
    }

    // Load Lịch sử quét thẻ RFID (Lấy 15 lượt quét mới nhất)
    const accessLogsContainer = document.getElementById("rfid-access-logs");
    if (accessLogsContainer) {
        const rfidAccessQuery = query(ref(db, "rfid/access_logs"), limitToLast(15));
        onValue(rfidAccessQuery, (snapshot) => {
            accessLogsContainer.innerHTML = "";
            
            const logs = snapshot.val();
            if (!logs) {
                accessLogsContainer.innerHTML = `<p class="text-center text-muted">Chưa có lịch sử quét thẻ.</p>`;
                return;
            }

            // Đảo ngược logs để cái mới nhất lên đầu
            const sortedKeys = Object.keys(logs).reverse();
            sortedKeys.forEach(key => {
                const log = logs[key];
                const date = new Date(log.timestamp);
                const timeStr = date.toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit', second: '2-digit' }) + " " + date.toLocaleDateString('vi-VN', { day: '2-digit', month: '2-digit' });
                
                const logItem = document.createElement("div");
                logItem.className = "rfid-log-item";
                
                let statusClass = log.status === "granted" ? "granted" : "denied";
                let statusText = log.status === "granted" ? "Hợp lệ" : "Từ chối";
                if (log.status === "inactive_denied") statusText = "Bị khóa";

                logItem.innerHTML = `
                    <div class="log-meta">
                        <span class="log-title">${escapeHTML(log.name)} <code>(${escapeHTML(log.card_uid)})</code></span>
                        <span class="log-subtitle">${escapeHTML(timeStr)}</span>
                    </div>
                    <span class="log-badge ${escapeHTML(statusClass)}">${escapeHTML(statusText)}</span>
                `;
                accessLogsContainer.appendChild(logItem);
            });
        });
    }
}
