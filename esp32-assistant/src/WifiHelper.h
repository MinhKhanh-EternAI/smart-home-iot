#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Firebase_ESP_Client.h>
#include "Config.h"

class WifiHelper {
private:
    static WebServer& getServer() {
        static WebServer server(80);
        return server;
    }

public:
    static bool& getAPMode() {
        static bool isAPMode = false;
        return isAPMode;
    }

private:
    // Ghi chuỗi vào EEPROM
    static void writeStringToEEPROM(int offset, const String& str) {
        int len = str.length();
        for (int i = 0; i < len; i++) {
            EEPROM.write(offset + i, str[i]);
        }
        EEPROM.write(offset + len, '\0'); // Ký tự kết thúc chuỗi
        EEPROM.commit();
    }

    // Đọc chuỗi từ EEPROM
    static String readStringFromEEPROM(int offset, int maxLen) {
        String str = "";
        for (int i = 0; i < maxLen; i++) {
            char c = EEPROM.read(offset + i);
            if (c == '\0' || c == 0xFF) break;
            str += c;
        }
        return str;
    }

public:
    static void init() {
        Serial.println("\n--- [WifiHelper] Bat dau khoi tao WiFi & EEPROM ---");
        EEPROM.begin(EEPROM_SIZE);
        Serial.println("[WifiHelper] EEPROM.begin() khoi tao thanh cong.");
        
        // Đọc thông tin WiFi đã lưu
        String savedSSID = readStringFromEEPROM(0, 32);
        String savedPass = readStringFromEEPROM(32, 64);
        
        Serial.printf("[WifiHelper] SSID doc tu EEPROM: \"%s\"\n", savedSSID.c_str());
        Serial.printf("[WifiHelper] Pass doc tu EEPROM: \"%s\"\n", savedPass.c_str());
        
        bool connected = false;
        
        if (savedSSID.length() > 0 && savedSSID != "Your_SSID" && savedSSID[0] != 0xFF) {
            WiFi.mode(WIFI_STA);
            WiFi.begin(savedSSID.c_str(), savedPass.c_str());
            
            Serial.printf("[WifiHelper] Dang ket noi Wi-Fi: %s ", savedSSID.c_str());
            
            int timeout = 0;
            while (WiFi.status() != WL_CONNECTED && timeout < 30) { // Đợi tối đa 15 giây
                delay(500);
                Serial.print(".");
                timeout++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                connected = true;
                Serial.println("\n[WifiHelper] Wi-Fi da ket noi thanh cong!");
                Serial.print("[WifiHelper] IP cua ESP32: "); Serial.println(WiFi.localIP());
                
                delay(2000);
            } else {
                Serial.println("\n[WifiHelper] Ket noi Wi-Fi that bai (Timeout)!");
            }
        } else {
            Serial.println("[WifiHelper] Khong co thong tin WiFi hop le hoac day la lan khoi dong dau.");
        }
        
        // Nếu không có thông tin hoặc kết nối thất bại -> Chuyển sang Access Point Mode để cấu hình
        if (!connected) {
            startAPConfigPortal();
        }
    }

    static void startAPConfigPortal() {
        Serial.println("\n--- [WifiHelper] Khoi chay AP Config Portal ---");
        getAPMode() = true;
        
        WiFi.disconnect();
        WiFi.mode(WIFI_AP);
        
        // Đặt IP tĩnh cho AP từ Config.h
        IPAddress apIP(AP_IP_ADDR);
        IPAddress gateway(AP_IP_ADDR);
        IPAddress subnet(AP_SUBNET_ADDR);
        WiFi.softAPConfig(apIP, gateway, subnet);

        // Tạo mạng WiFi không mật khẩu
        WiFi.softAP(AP_SSID);
        Serial.printf("[WifiHelper] AP Mode khoi dong. SSID: %s\n", AP_SSID);
        Serial.print("[WifiHelper] AP IP Address: "); Serial.println(WiFi.softAPIP());
        
        // Thiết lập các Endpoint cho Web Server
        WebServer& server = getServer();
        
        // Trang chủ cấu hình WiFi
        server.on("/", HTTP_GET, []() {
            String html = 
                "<!DOCTYPE html><html><head><meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                "<title>SmartHome Config Portal</title>"
                "<style>"
                ":root {"
                "  --primary: #0ea5e9;"
                "  --primary-glow: rgba(14, 165, 233, 0.2);"
                "  --primary-hover: #0284c7;"
                "  --bg-gradient: linear-gradient(135deg, #0f172a 0%, #1e1e38 50%, #090d16 100%);"
                "  --panel-bg: rgba(22, 28, 45, 0.45);"
                "  --panel-border: rgba(255, 255, 255, 0.08);"
                "  --panel-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);"
                "  --text-primary: #f8fafc;"
                "  --text-secondary: #94a3b8;"
                "}"
                "body { font-family: 'Outfit', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #0b0f19; color: var(--text-primary); display: flex; align-items: center; justify-content: center; min-height: 100vh; margin: 0; padding: 16px; box-sizing: border-box; overflow-x: hidden; }"
                ".glass-bg { position: fixed; top: 0; left: 0; right: 0; bottom: 0; background: var(--bg-gradient); z-index: -2; overflow: hidden; }"
                ".glass-bg::after { content: ''; position: absolute; top: 20%; left: 10%; width: 300px; height: 300px; background: radial-gradient(circle, var(--primary-glow) 0%, rgba(14, 165, 233, 0) 70%); filter: blur(50px); z-index: -1; animation: bgPulse 15s infinite alternate; }"
                ".glass-bg::before { content: ''; position: absolute; bottom: 10%; right: 15%; width: 400px; height: 400px; background: radial-gradient(circle, rgba(99, 102, 241, 0.15) 0%, rgba(99, 102, 241, 0) 70%); filter: blur(60px); z-index: -1; animation: bgPulse 20s infinite alternate-reverse; }"
                "@keyframes bgPulse { 0% { transform: translate(0, 0) scale(1); } 100% { transform: translate(50px, 30px) scale(1.2); } }"
                ".card { background: var(--panel-bg); border: 1px solid var(--panel-border); backdrop-filter: blur(12px); -webkit-backdrop-filter: blur(12px); border-radius: 16px; padding: 32px 24px; width: 100%; max-width: 420px; box-shadow: var(--panel-shadow); text-align: center; position: relative; overflow: hidden; }"
                ".content-wrapper { position: relative; z-index: 1; }"
                ".chip-icon { display: inline-block; margin-bottom: 12px; animation: pulse 2s infinite ease-in-out; }"
                "@keyframes pulse {"
                "  0%, 100% { transform: scale(1); filter: drop-shadow(0 0 5px var(--primary)); }"
                "  50% { transform: scale(1.08); filter: drop-shadow(0 0 15px var(--primary)); }"
                "}"
                "h2 { margin: 0 0 6px 0; font-weight: 600; font-size: 24px; letter-spacing: -0.5px; background: linear-gradient(to right, #ffffff, #cbd5e1); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }"
                ".badge { display: inline-block; padding: 6px 14px; border-radius: 50px; font-size: 12px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.5px; background: rgba(14, 165, 233, 0.1); border: 1px solid rgba(14, 165, 233, 0.3); margin-bottom: 20px; color: #bae6fd; box-shadow: 0 2px 10px rgba(0,0,0,0.2); }"
                "p { color: var(--text-secondary); font-size: 14px; line-height: 1.5; margin: 0 0 24px 0; }"
                ".btn { background: var(--primary); color: white; border: 0; padding: 14px 20px; border-radius: 10px; font-size: 14px; font-weight: 600; cursor: pointer; width: 100%; transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1); box-shadow: 0 4px 14px var(--primary-glow); margin-bottom: 16px; box-sizing: border-box; outline: none; }"
                ".btn:hover { background: var(--primary-hover); transform: translateY(-2px); box-shadow: 0 6px 20px var(--primary-glow); }"
                ".btn:active { transform: translateY(0); }"
                ".btn:disabled { opacity: 0.5; cursor: not-allowed; transform: none; }"
                ".btn-scan { background: rgba(255, 255, 255, 0.04); border: 1px solid rgba(255, 255, 255, 0.08); box-shadow: none; color: #e2e8f0; }"
                ".btn-scan:hover { background: rgba(255, 255, 255, 0.08); color: white; box-shadow: none; }"
                "select, input { width: 100%; padding: 12px 16px; background: rgba(0, 0, 0, 0.2); border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 10px; color: white; font-size: 14px; margin-bottom: 16px; box-sizing: border-box; outline: none; transition: all 0.3s; }"
                "select:focus, input:focus { border-color: var(--primary); box-shadow: 0 0 0 3px var(--primary-glow); }"
                "option { background: #0f172a; color: white; }"
                ".loading { display: none; margin: 12px 0 20px 0; font-size: 13px; color: var(--primary); font-weight: 500; }"
                ".success-msg { display: none; color: #10b981; font-weight: 600; margin-top: 16px; padding: 12px; border-radius: 10px; background: rgba(16, 185, 129, 0.1); border: 1px solid rgba(16, 185, 129, 0.2); font-size: 14px; line-height: 1.4; }"
                ".version-info { font-size: 10px; color: #475569; margin-top: 24px; }"
                "</style></head><body>"
                "<div class='glass-bg'></div>"
                "<div class='card'>"
                "<div class='content-wrapper'>"
                "<div class='chip-icon'>"
                "<svg width='48' height='48' viewBox='0 0 24 24' fill='none' stroke='#38bdf8' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><rect x='4' y='4' width='16' height='16' rx='2'/><rect x='9' y='9' width='6' height='6'/><path d='M9 1v3M15 1v3M9 20v3M15 20v3M20 9h3M20 15h3M1 9h3M1 15h3'/></svg>"
                "</div>"
                "<h2>SmartHome Config</h2>"
                "<div class='badge'>ESP32 Assistant • Trợ Lý Giọng Nói</div>"
                "<p>Cấu hình kết nối Wi-Fi để thiết bị này liên kết vào hệ thống Smart Home.</p>"
                "<button class='btn btn-scan' id='btn-scan' onclick='scanNetworks()'>Quét mạng Wi-Fi xung quanh</button>"
                "<div class='loading' id='loading'>Đang tìm kiếm mạng Wi-Fi...</div>"
                "<select id='ssid-select' style='display: none;' onchange='onSelectChange()'>"
                "<option value=''>-- Chọn mạng Wi-Fi --</option>"
                "</select>"
                "<input type='text' id='ssid-input' placeholder='Nhập tên SSID thủ công'>"
                "<input type='password' id='password' placeholder='Nhập mật khẩu Wi-Fi'>"
                "<button class='btn' onclick='save()'>Lưu & Kết nối</button>"
                "<div class='success-msg' id='success'>"
                "  <b>Lưu cấu hình thành công!</b><br>"
                "  Thiết bị đang tự động khởi động lại để kết nối mạng mới..."
                "</div>"
                "<div class='version-info'>v1.1.0 • Smart Home IoT</div>"
                "</div>"
                "</div>"
                "<script>"
                "function scanNetworks() {"
                "  var btn = document.getElementById('btn-scan');"
                "  var loading = document.getElementById('loading');"
                "  var select = document.getElementById('ssid-select');"
                "  btn.disabled = true;"
                "  loading.style.display = 'block';"
                "  select.style.display = 'none';"
                "  fetch('/scan')"
                "    .then(res => res.json())"
                "    .then(data => {"
                "      select.innerHTML = '<option value=\"\">-- Chọn mạng Wi-Fi --</option>';"
                "      if(data && data.length > 0) {"
                "        data.forEach(net => {"
                "          var opt = document.createElement('option');"
                "          opt.value = net.ssid;"
                "          opt.innerText = net.ssid + ' (' + net.rssi + ' dBm)';"
                "          select.appendChild(opt);"
                "        });"
                "        select.style.display = 'block';"
                "      }"
                "    })"
                "    .catch(err => alert('Lỗi quét WiFi!'))"
                "    .finally(() => {"
                "      btn.disabled = false;"
                "      loading.style.display = 'none';"
                "    });"
                "}"
                "function onSelectChange() {"
                "  var select = document.getElementById('ssid-select');"
                "  var input = document.getElementById('ssid-input');"
                "  input.value = select.value;"
                "}"
                "function save() {"
                "  var ssid = document.getElementById('ssid-input').value.trim();"
                "  var password = document.getElementById('password').value;"
                "  if(!ssid) { alert('Vui lòng chọn hoặc tự nhập tên WiFi!'); return; }"
                "  var formData = new FormData();"
                "  formData.append('ssid', ssid);"
                "  formData.append('password', password);"
                "  fetch('/save', { method: 'POST', body: formData })"
                "    .then(res => {"
                "      document.getElementById('success').style.display = 'block';"
                "    })"
                "    .catch(err => alert('Lỗi lưu cấu hình!'));"
                "}"
                "</script></body></html>";
            getServer().send(200, "text/html", html);
        });

        // Endpoint quét mạng WiFi xung quanh
        server.on("/scan", HTTP_GET, []() {
            Serial.println("API Request: Scan Wi-Fi");
            int n = WiFi.scanNetworks();
            String json = "[";
            for (int i = 0; i < n; ++i) {
                if (i > 0) json += ",";
                json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
            }
            json += "]";
            getServer().send(200, "application/json", json);
        });

        // Endpoint lưu cấu hình và khởi động lại
        server.on("/save", HTTP_POST, []() {
            String ssid = getServer().arg("ssid");
            String password = getServer().arg("password");
            
            Serial.println("API Request: Save Wi-Fi Configuration");
            Serial.print("SSID: "); Serial.println(ssid);
            
            // Lưu vào EEPROM
            writeStringToEEPROM(0, ssid);
            writeStringToEEPROM(32, password);
            
            getServer().send(200, "application/json", "{\"status\":\"ok\"}");
            
            delay(2000);
            ESP.restart();
        });

        server.begin();
        Serial.println("Web Server initialized and listening on port 80.");
    }

    static void handleClient() {
        if (getAPMode()) {
            getServer().handleClient();
        }
    }

    static void keepAlive() {
        // Chỉ chạy keepAlive khi không ở chế độ cấu hình AP
        if (!getAPMode() && WiFi.status() != WL_CONNECTED) {
            static unsigned long lastReconnectAttempt = 0;
            unsigned long now = millis();
            if (now - lastReconnectAttempt > 15000) { // Thử kết nối lại mỗi 15 giây
                lastReconnectAttempt = now;
                Serial.println("WiFi disconnected. Reconnecting...");
                
                String savedSSID = readStringFromEEPROM(0, 32);
                String savedPass = readStringFromEEPROM(32, 64);
                
                if (savedSSID.length() > 0 && savedSSID != "Your_SSID" && savedSSID[0] != 0xFF) {
                    WiFi.disconnect();
                    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
                }
            }
        }
    }
};

#endif // WIFI_HELPER_H
