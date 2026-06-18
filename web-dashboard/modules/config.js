import { db, showToast } from "../app.js";
import { ref, onValue, set } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

export function initConfig() {
    // WiFi Config
    onValue(ref(db, "config/wifi"), (snapshot) => {
        const wifi = snapshot.val();
        if (wifi) {
            const ssidEl = document.getElementById("wifi-ssid");
            const passEl = document.getElementById("wifi-pass");
            if (ssidEl) ssidEl.value = wifi.ssid || "";
            if (passEl) passEl.value = wifi.password || "";
        }
    });

    const wifiForm = document.getElementById("wifi-config-form");
    if (wifiForm) {
        wifiForm.addEventListener("submit", (e) => {
            e.preventDefault();
            const ssid = document.getElementById("wifi-ssid").value.trim();
            const pass = document.getElementById("wifi-pass").value.trim();
            
            set(ref(db, "config/wifi"), {
                ssid: ssid,
                password: pass
            }).then(() => {
                showToast("Cập nhật WiFi thành công! Thiết bị sẽ tự kết nối lại.", "success");
            });
        });
    }

    // MCP AI Config
    onValue(ref(db, "config/mcp"), (snapshot) => {
        const mcp = snapshot.val();
        if (mcp) {
            const endpointEl = document.getElementById("mcp-endpoint");
            if (endpointEl) endpointEl.value = mcp.endpoint || "";
        }
    });

    const mcpForm = document.getElementById("mcp-config-form");
    if (mcpForm) {
        mcpForm.addEventListener("submit", (e) => {
            e.preventDefault();
            const endpoint = document.getElementById("mcp-endpoint").value.trim();
            
            set(ref(db, "config/mcp/endpoint"), endpoint).then(() => {
                showToast("Cập nhật Endpoint Xiaozhi MCP thành công!", "success");
            });
        });
    }

    // Cấu hình Tools JSON
    const jsonEditor = document.getElementById("tools-json-editor");
    const jsonError = document.getElementById("json-error-msg");

    onValue(ref(db, "config/tools"), (snapshot) => {
        const tools = snapshot.val();
        if (tools && jsonEditor) {
            jsonEditor.value = JSON.stringify(tools, null, 2);
        }
    });

    const saveToolsBtn = document.getElementById("btn-save-tools-json");
    if (saveToolsBtn && jsonEditor) {
        saveToolsBtn.addEventListener("click", () => {
            const rawVal = jsonEditor.value.trim();
            try {
                const parsed = JSON.parse(rawVal);
                if (jsonError) jsonError.style.display = "none";
                
                // Lưu lên Firebase
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
