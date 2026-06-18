#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include "Config.h"

class WifiHelper {
private:
    static ESP8266WebServer& getServer() {
        static ESP8266WebServer server(80);
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
    static int& getLCDAddress() {
        static int lcdAddress = -1;
        return lcdAddress;
    }

    static void printToLCD(const String& line1, const String& line2 = "") {
        int addr = getLCDAddress();
        if (addr != -1) {
            Serial.printf("[LCD] Writing -> L1: \"%s\", L2: \"%s\"\n", line1.c_str(), line2.c_str());
            LiquidCrystal_I2C lcd(addr, 16, 2);
            lcd.init();
            lcd.backlight();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(line1.substring(0, 16));
            if (line2.length() > 0) {
                lcd.setCursor(0, 1);
                lcd.print(line2.substring(0, 16));
            }
        } else {
            Serial.printf("[LCD-Skip] LCD not detected. Message: L1: \"%s\", L2: \"%s\"\n", line1.c_str(), line2.c_str());
        }
    }

    static void init() {
        Serial.println("\n--- [WifiHelper] Bat dau khoi tao WiFi & EEPROM ---");
        EEPROM.begin(EEPROM_SIZE);
        Serial.println("[WifiHelper] EEPROM.begin() khoi tao thanh cong.");
        
        // Đọc thông tin WiFi đã lưu
        String savedSSID = readStringFromEEPROM(0, 32);
        String savedPass = readStringFromEEPROM(32, 64);
        
        Serial.printf("[WifiHelper] SSID doc tu EEPROM: \"%s\"\n", savedSSID.c_str());
        Serial.printf("[WifiHelper] Pass doc tu EEPROM: \"%s\"\n", savedPass.c_str());
        
        // Khởi tạo I2C Bus với các chân cấu hình trước khi detect LCD
        Serial.printf("[WifiHelper] Khoi dong I2C Bus... SDA Pin: %d, SCL Pin: %d\n", I2C_SDA_PIN, I2C_SCL_PIN);
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        
        // Dò tìm LCD trên bus I2C để tránh treo chip khi phần cứng bị lỏng/thiếu nguồn
        Serial.println("[WifiHelper] Quet I2C bus tim LCD (0x27, 0x3F, 0x20)...");
        int lcdAddr = -1;
        byte addresses[] = {0x27, 0x3F, 0x20};
        for (int i = 0; i < 3; i++) {
            Wire.beginTransmission(addresses[i]);
            byte error = Wire.endTransmission();
            if (error == 0) {
                lcdAddr = addresses[i];
                Serial.printf("[WifiHelper] Tim thay LCD tai dia chi I2C: 0x%02X\n", lcdAddr);
                break;
            } else {
                Serial.printf("[WifiHelper] Dia chi 0x%02X khong phan hoi (Error: %d)\n", addresses[i], error);
            }
        }
        getLCDAddress() = lcdAddr;
        if (lcdAddr == -1) {
            Serial.println("[WifiHelper] CANH BAO: Khong tim thay LCD. Bo qua hien thi LCD.");
        }
        
        bool connected = false;
        
        if (savedSSID.length() > 0 && savedSSID != "Your_SSID" && savedSSID[0] != 0xFF) {
            printToLCD("Connecting WiFi", savedSSID);
            
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
                Serial.print("[WifiHelper] IP cua ESP8266: "); Serial.println(WiFi.localIP());
                
                printToLCD("WiFi Connected!", WiFi.localIP().toString());
                delay(2000); // Giữ thông tin IP trong 2 giây
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
        
        // Tạo mạng WiFi không mật khẩu
        WiFi.softAP(AP_SSID);
        Serial.printf("[WifiHelper] AP Mode khoi dong. SSID: %s\n", AP_SSID);
        Serial.print("[WifiHelper] AP IP Address: "); Serial.println(WiFi.softAPIP());
        
        // Hiển thị thông tin AP lên LCD
        printToLCD("AP: SmartHome", "IP: 192.168.4.1");
        
        // Thiết lập các Endpoint cho Web Server
        ESP8266WebServer& server = getServer();
        
        // Trang chủ cấu hình WiFi
        server.on("/", HTTP_GET, []() {
            String html = 
                "<!DOCTYPE html><html><head><meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                "<title>SmartHome Config</title>"
                "<style>"
                "body { font-family: sans-serif; background: linear-gradient(135deg, #0f172a 0%, #1e1b4b 100%); color: #f8fafc; display: flex; align-items: center; justify-content: center; min-height: 100vh; margin: 0; padding: 16px; box-sizing: border-box; }"
                ".card { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.08); backdrop-filter: blur(16px); border-radius: 16px; padding: 24px; width: 100%; max-width: 400px; box-shadow: 0 20px 40px rgba(0,0,0,0.5); text-align: center; }"
                "h2 { margin-top: 0; font-weight: 600; background: linear-gradient(to right, #6366f1, #a855f7); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }"
                "p { color: #94a3b8; font-size: 14px; margin-bottom: 24px; }"
                ".btn { background: #6366f1; color: white; border: 0; padding: 12px 20px; border-radius: 8px; font-size: 14px; font-weight: 600; cursor: pointer; width: 100%; transition: all 0.2s ease; box-shadow: 0 4px 12px rgba(99, 102, 241, 0.3); margin-bottom: 16px; }"
                ".btn:hover { background: #4f46e5; transform: translateY(-1px); }"
                ".btn-scan { background: rgba(255,255,255,0.05); border: 1px solid rgba(255,255,255,0.1); box-shadow: none; }"
                ".btn-scan:hover { background: rgba(255,255,255,0.1); }"
                "select, input { width: 100%; padding: 12px; background: rgba(255, 255, 255, 0.05); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; color: white; font-size: 14px; margin-bottom: 16px; box-sizing: border-box; outline: none; }"
                "option { background: #0f172a; color: white; }"
                ".loading { display: none; margin: 12px 0; font-size: 13px; color: #a855f7; }"
                ".success-msg { display: none; color: #10b981; font-weight: 600; margin-top: 16px; }"
                "</style></head><body>"
                "<div class='card'>"
                "<h2>SmartHome Config</h2>"
                "<p>Cấu hình mạng Wi-Fi để thiết bị kết nối vào hệ thống.</p>"
                "<button class='btn btn-scan' id='btn-scan' onclick='scanNetworks()'>Quét mạng Wi-Fi xung quanh</button>"
                "<div class='loading' id='loading'>Đang quét mạng Wi-Fi...</div>"
                "<select id='ssid-select' style='display: none;' onchange='onSelectChange()'>"
                "<option value=''>-- Chọn mạng Wi-Fi --</option>"
                "</select>"
                "<input type='text' id='ssid-input' placeholder='Nhập tên SSID thủ công'>"
                "<input type='password' id='password' placeholder='Nhập mật khẩu Wi-Fi'>"
                "<button class='btn' onclick='save()'>Lưu & Kết nối</button>"
                "<div class='success-msg' id='success'>Đã lưu cấu hình thành công! Thiết bị đang tự động khởi động lại...</div>"
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
            
            // Hiển thị LCD thông báo lưu thành công và khởi động lại
            printToLCD("WiFi Config Saved", "Rebooting...");
            
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
