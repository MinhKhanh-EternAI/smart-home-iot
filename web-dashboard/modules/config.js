let db;
let showToast;
import { ref, onValue, set, remove } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";

export function initConfig(database, toastFn) {
    db = database;
    showToast = toastFn;

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
