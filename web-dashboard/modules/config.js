import { db, showToast } from "../app.js";
import { ref, onValue, set, remove } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

export function initConfig() {
    // WiFi Config — chỉ hiện SSID, không pre-fill mật khẩu (bảo mật)
    onValue(ref(db, "config/wifi"), (snapshot) => {
        const wifi = snapshot.val();
        const ssidEl = document.getElementById("wifi-ssid");
        const passEl = document.getElementById("wifi-pass");
        const apNoteEl = document.getElementById("wifi-ap-note");

        if (ssidEl) ssidEl.value = wifi?.ssid || "";
        if (passEl) {
            passEl.value = "";
            passEl.placeholder = wifi?.ssid ? "Nhập mật khẩu mới để thay đổi..." : "Nhập mật khẩu WiFi...";
        }
        if (apNoteEl) apNoteEl.style.display = wifi?.ssid ? "none" : "flex";
    });

    const wifiForm = document.getElementById("wifi-config-form");
    if (wifiForm) {
        wifiForm.addEventListener("submit", (e) => {
            e.preventDefault();
            const ssid = document.getElementById("wifi-ssid").value.trim();
            const pass = document.getElementById("wifi-pass").value.trim();
            if (!pass) {
                showToast("Vui lòng nhập mật khẩu WiFi.", "error");
                return;
            }
            set(ref(db, "config/wifi"), { ssid, password: pass }).then(() => {
                document.getElementById("wifi-pass").value = "";
                showToast("Đã gửi cấu hình WiFi lên Firebase. Thiết bị sẽ kết nối lại khi online.", "success");
            }).catch(err => showToast("Lỗi: " + err.message, "error"));
        });
    }

    // MCP AI Config
    onValue(ref(db, "config/mcp"), (snapshot) => {
        const mcp = snapshot.val();
        const endpointEl = document.getElementById("mcp-endpoint");
        if (endpointEl) endpointEl.value = mcp?.endpoint || "";
    });

    const mcpForm = document.getElementById("mcp-config-form");
    if (mcpForm) {
        mcpForm.addEventListener("submit", (e) => {
            e.preventDefault();
            const endpoint = document.getElementById("mcp-endpoint").value.trim();
            set(ref(db, "config/mcp/endpoint"), endpoint).then(() => {
                showToast("Cập nhật Endpoint Xiaozhi MCP thành công!", "success");
            }).catch(err => showToast("Lỗi: " + err.message, "error"));
        });
    }

    // Cấu hình Tools JSON
    const jsonEditor = document.getElementById("tools-json-editor");
    const jsonError = document.getElementById("json-error-msg");

    onValue(ref(db, "config/tools"), (snapshot) => {
        const tools = snapshot.val();
        if (jsonEditor) jsonEditor.value = tools ? JSON.stringify(tools, null, 2) : "";
    });

    const saveToolsBtn = document.getElementById("btn-save-tools-json");
    if (saveToolsBtn && jsonEditor) {
        saveToolsBtn.addEventListener("click", () => {
            const rawVal = jsonEditor.value.trim();
            if (!rawVal) {
                remove(ref(db, "config/tools")).then(() => showToast("Đã xoá cấu hình Tools.", "info"));
                return;
            }
            try {
                const parsed = JSON.parse(rawVal);
                if (jsonError) jsonError.style.display = "none";
                set(ref(db, "config/tools"), parsed).then(() => {
                    showToast("Lưu cấu hình Tools JSON lên Firebase thành công!", "success");
                });
            } catch (e) {
                if (jsonError) {
                    jsonError.style.display = "block";
                    jsonError.innerText = "Lỗi cú pháp JSON: " + e.message;
                }
            }
        });
    }
}
