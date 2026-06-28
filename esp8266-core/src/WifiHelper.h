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

    static void checkWiFiResetButton() {
        pinMode(0, INPUT_PULLUP);
        delay(10);
        if (digitalRead(0) == LOW) {
            Serial.println("[WifiHelper] Phat hien nhan nut BOOT! Giu 3s de reset WiFi...");
            unsigned long pressed = millis();
            while (millis() - pressed < 3000) {
                delay(10);
                if (digitalRead(0) != LOW) {
                    Serial.println("[WifiHelper] Nha nut som -> bo qua.");
                    return;
                }
            }
            Serial.println("[WifiHelper] Xoa WiFi credentials...");
            for (int i = 0; i < EEPROM_SIZE; i++) {
                EEPROM.write(i, 0);
            }
            EEPROM.commit();
            WiFi.disconnect(true);
            Serial.println("[WifiHelper] WiFi da xoa! Nha nut BOOT de reboot...");
            while (digitalRead(0) == LOW) {
                delay(10);
            }
            delay(200);
            Serial.println("[WifiHelper] Dang reboot vao AP mode...");
            ESP.restart();
        }
    }

public:
    static int& getLCDAddress() {
        static int lcdAddress = -1;
        return lcdAddress;
    }

    static LiquidCrystal_I2C& getLCD() {
        static LiquidCrystal_I2C lcd(getLCDAddress() != -1 ? getLCDAddress() : 0x27, 16, 2);
        return lcd;
    }

    static void printToLCD(const String& line1, const String& line2 = "") {
        int addr = getLCDAddress();
        if (addr != -1) {
            if (Serial) Serial.printf("[LCD] Writing -> L1: \"%s\", L2: \"%s\"\n", line1.c_str(), line2.c_str());
            LiquidCrystal_I2C& lcd = getLCD();
            static bool lcdInited = false;
            if (!lcdInited) {
                lcd.init();
                lcd.backlight();
                lcdInited = true;
            }
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(line1.substring(0, 16));
            if (line2.length() > 0) {
                lcd.setCursor(0, 1);
                lcd.print(line2.substring(0, 16));
            }
        } else {
            if (Serial) Serial.printf("[LCD-Skip] LCD not detected. Message: L1: \"%s\", L2: \"%s\"\n", line1.c_str(), line2.c_str());
        }
    }

    // ===== LCD Notification & Standby System =====
    static String& getNotifLine1() {
        static String line1 = ""; return line1;
    }
    static String& getNotifLine2() {
        static String line2 = ""; return line2;
    }
    static unsigned long& getNotifStart() {
        static unsigned long start = 0; return start;
    }
    static unsigned long& getNotifDuration() {
        static unsigned long dur = 5000; return dur;
    }
    static bool& getHasNotification() {
        static bool has = false; return has;
    }

    static float& getStandbyTemp() {
        static float t = -999.0f; return t;
    }
    static float& getStandbyHum() {
        static float h = -999.0f; return h;
    }
    static unsigned long& getLastStandbySwitch() {
        static unsigned long t = 0; return t;
    }
    static int& getStandbyPage() {
        static int p = 0; return p;
    }

    static void showNotification(const String& line1, const String& line2, unsigned long durationMs = 5000) {
        getNotifLine1() = line1;
        getNotifLine2() = line2;
        getNotifStart() = millis();
        getNotifDuration() = durationMs;
        getHasNotification() = true;
        printToLCD(line1, line2);
    }

    static void setStandbyData(float temp, float hum) {
        getStandbyTemp() = temp;
        getStandbyHum() = hum;
    }

    static void showStandbyScreen() {
        if (getAPMode()) {
            printToLCD("AP: " + String(AP_SSID), "IP: " + WiFi.softAPIP().toString());
            return;
        }
        unsigned long now = millis();
        if (now - getLastStandbySwitch() >= 4000) {
            getLastStandbySwitch() = now;
            getStandbyPage() = (getStandbyPage() + 1) % 2;
        }
        if (WiFi.status() == WL_CONNECTED) {
            if (getStandbyPage() == 0) {
                float t = getStandbyTemp();
                float h = getStandbyHum();
                if (t > -998.0f) {
                    char buf[17];
                    snprintf(buf, sizeof(buf), "T:%.1fC H:%.0f%%", t, h);
                    printToLCD(String(buf), "WiFi:Connected");
                } else {
                    printToLCD("Smart Home IoT", "WiFi:Connected");
                }
            } else {
                printToLCD("IP: " + WiFi.localIP().toString(), "WiFi:Connected");
            }
        } else {
            printToLCD("Connecting WiFi...", "");
        }
    }

    static void updateLCD() {
        if (getHasNotification()) {
            if (millis() - getNotifStart() >= getNotifDuration()) {
                getHasNotification() = false;
                showStandbyScreen();
            }
        }
    }

    static void init() {
        Serial.println("\n--- [WifiHelper] Bat dau khoi tao WiFi & EEPROM ---");
        EEPROM.begin(EEPROM_SIZE);
        Serial.println("[WifiHelper] EEPROM.begin() khoi tao thanh cong.");

        // Kiểm tra version firmware — nếu không khớp thì xoá EEPROM (flash mới)
        uint32_t storedVersion;
        EEPROM.get(EEPROM_VERSION_OFFSET, storedVersion);
        if (storedVersion != FIRMWARE_VERSION) {
            Serial.printf("[WifiHelper] FIRMWARE_VERSION thay doi (%lu -> %d). Xoa EEPROM...\n",
                          (unsigned long)storedVersion, FIRMWARE_VERSION);
            for (int i = 0; i < EEPROM_SIZE; i++) {
                EEPROM.write(i, 0);
            }
            EEPROM.put(EEPROM_VERSION_OFFSET, (uint32_t)FIRMWARE_VERSION);
            EEPROM.commit();
            Serial.println("[WifiHelper] EEPROM da duoc xoa va ghi version moi.");
        } else {
            Serial.printf("[WifiHelper] FIRMWARE_VERSION %d — giu nguyen EEPROM.\n", FIRMWARE_VERSION);
        }

        // Giữ nút FLASH (GPIO0/D3) trong 3 giây khi khởi động để xóa WiFi
        checkWiFiResetButton();

        // Đọc thông tin WiFi đã lưu
        String savedSSID = readStringFromEEPROM(0, 32);
        String savedPass = readStringFromEEPROM(32, 64);
        
        Serial.printf("[WifiHelper] SSID doc tu EEPROM: \"%s\"\n", savedSSID.c_str());
        Serial.printf("[WifiHelper] Pass doc tu EEPROM: \"%s\"\n", savedPass.c_str());
        
        // Khởi tạo I2C Bus với các chân cấu hình trước khi detect LCD
        Serial.printf("[WifiHelper] Khoi dong I2C Bus... SDA Pin: %d, SCL Pin: %d\n", I2C_SDA_PIN, I2C_SCL_PIN);
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        
        // Dò tìm LCD trên bus I2C để tránh treo chip khi phần cứng bị lỏng/thiếu nguồn
        Serial.println("[WifiHelper] Quet I2C bus tim LCD (0x27, 0x3F)...");
        int lcdAddr = -1;
        byte addresses[] = {0x27, 0x3F};
        for (size_t i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++) {
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
        
                if (savedSSID.length() > 0 && savedSSID != "Your_SSID" && (unsigned char)savedSSID[0] != 0xFF) {
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
        WiFi.mode(WIFI_AP_STA);
        
        // Đặt IP tĩnh cho AP từ Config.h
        IPAddress apIP(AP_IP_ADDR);
        IPAddress gateway(AP_IP_ADDR);
        IPAddress subnet(AP_SUBNET_ADDR);
        WiFi.softAPConfig(apIP, gateway, subnet);

        // Tạo mạng WiFi không mật khẩu
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        Serial.printf("[WifiHelper] AP Mode khoi dong. SSID: %s\n", AP_SSID);
        Serial.print("[WifiHelper] AP IP Address: "); Serial.println(WiFi.softAPIP());
        

        
        // Thiết lập các Endpoint cho Web Server
        ESP8266WebServer& server = getServer();
        
        // Trang chủ cấu hình WiFi
        server.on("/", HTTP_GET, []() {
            String html = 
                "<!DOCTYPE html><html><head><meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                "<title>SmartHome Config Portal</title>"
                "<style>"
                ":root {"
                "  --primary: #6366f1;"
                "  --primary-glow: rgba(99, 102, 241, 0.2);"
                "  --primary-hover: #4f46e5;"
                "  --bg-gradient: linear-gradient(135deg, #0f172a 0%, #1e1e38 50%, #090d16 100%);"
                "  --panel-bg: rgba(22, 28, 45, 0.45);"
                "  --panel-border: rgba(255, 255, 255, 0.08);"
                "  --panel-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);"
                "  --text-primary: #f8fafc;"
                "  --text-secondary: #94a3b8;"
                "}"
                "body { font-family: 'Outfit', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #0b0f19; color: var(--text-primary); display: flex; align-items: center; justify-content: center; min-height: 100vh; margin: 0; padding: 16px; box-sizing: border-box; overflow-x: hidden; }"
                ".glass-bg { position: fixed; top: 0; left: 0; right: 0; bottom: 0; background: var(--bg-gradient); z-index: -2; overflow: hidden; }"
                ".glass-bg::after { content: ''; position: absolute; top: 20%; left: 10%; width: 300px; height: 300px; background: radial-gradient(circle, var(--primary-glow) 0%, rgba(99, 102, 241, 0) 70%); filter: blur(50px); z-index: -1; animation: bgPulse 15s infinite alternate; }"
                ".glass-bg::before { content: ''; position: absolute; bottom: 10%; right: 15%; width: 400px; height: 400px; background: radial-gradient(circle, rgba(168, 85, 247, 0.15) 0%, rgba(168, 85, 247, 0) 70%); filter: blur(60px); z-index: -1; animation: bgPulse 20s infinite alternate-reverse; }"
                "@keyframes bgPulse { 0% { transform: translate(0, 0) scale(1); } 100% { transform: translate(50px, 30px) scale(1.2); } }"
                ".card { background: var(--panel-bg); border: 1px solid var(--panel-border); backdrop-filter: blur(12px); -webkit-backdrop-filter: blur(12px); border-radius: 16px; padding: 32px 24px; width: 100%; max-width: 420px; box-shadow: var(--panel-shadow); text-align: center; position: relative; overflow: hidden; }"
                ".content-wrapper { position: relative; z-index: 1; }"
                ".chip-icon { display: inline-block; margin-bottom: 12px; animation: pulse 2s infinite ease-in-out; }"
                "@keyframes pulse {"
                "  0%, 100% { transform: scale(1); filter: drop-shadow(0 0 5px var(--primary)); }"
                "  50% { transform: scale(1.08); filter: drop-shadow(0 0 15px var(--primary)); }"
                "}"
                "h2 { margin: 0 0 6px 0; font-weight: 600; font-size: 24px; letter-spacing: -0.5px; background: linear-gradient(to right, #ffffff, #cbd5e1); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }"
                ".badge { display: inline-block; padding: 6px 14px; border-radius: 50px; font-size: 12px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.5px; background: rgba(168, 85, 247, 0.1); border: 1px solid rgba(168, 85, 247, 0.3); margin-bottom: 20px; color: #d8b4fe; box-shadow: 0 2px 10px rgba(0,0,0,0.2); }"
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
                "<svg width='48' height='48' viewBox='0 0 24 24' fill='none' stroke='#c084fc' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><rect x='4' y='4' width='16' height='16' rx='2'/><rect x='9' y='9' width='6' height='6'/><path d='M9 1v3M15 1v3M9 20v3M15 20v3M20 9h3M20 15h3M1 9h3M1 15h3'/></svg>"
                "</div>"
                "<h2>SmartHome Config</h2>"
                "<div class='badge'>ESP8266 Core • Mạch Điều Khiển</div>"
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
            
            // Hiển thị LCD thông báo lưu thành công và khởi động lại
            printToLCD("WiFi Config Saved", "Rebooting...");
            
            delay(2000);
            ESP.restart();
        });

        // Endpoint xóa WiFi (factory reset)
        server.on("/reset", HTTP_GET, []() {
            String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Reset WiFi</title><style>body{font-family:sans-serif;background:#0f172a;color:#f8fafc;display:flex;align-items:center;justify-content:center;height:100vh;}.card{background:#1e293b;padding:32px;border-radius:16px;text-align:center;max-width:400px;}.btn{background:#ef4444;color:white;border:0;padding:12px 24px;border-radius:8px;cursor:pointer;font-size:16px;margin-top:16px;}.btn:hover{background:#dc2626;}</style></head><body><div class='card'><h2>Reset WiFi?</h2><p>Thiet bi se xoa WiFi va khoi dong lai.</p><button class='btn' onclick='fetch(\"/reset\",{method:\"POST\"}).then(()=>alert(\"WiFi da xoa! Dang reboot...\"));'>Xac nhan Reset</button></div></body></html>";
            getServer().send(200, "text/html", html);
        });
        server.on("/reset", HTTP_POST, []() {
            getServer().send(200, "application/json", "{\"status\":\"ok\"}");
            printToLCD("WiFi Reset!", "Rebooting...");
            for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
            EEPROM.commit();
            WiFi.disconnect(true);
            delay(1000);
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
                if (Serial) Serial.println("WiFi disconnected. Reconnecting...");
                
                String savedSSID = readStringFromEEPROM(0, 32);
                String savedPass = readStringFromEEPROM(32, 64);
                
                if (savedSSID.length() > 0 && savedSSID != "Your_SSID" && (unsigned char)savedSSID[0] != 0xFF) {
                    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
                }
            }
        }
    }

    static void checkResetButton() {
        static bool wasPressed = false;
        static unsigned long pressedTime = 0;
        static unsigned long lastDebounceTime = 0;
        static bool lastReading = HIGH;
        static int lastReportedSec = 0;
        static bool pinInitialized = false;

        if (!pinInitialized) {
            pinMode(0, INPUT_PULLUP);
            pinInitialized = true;
        }

        bool reading = digitalRead(0);

        if (reading != lastReading) {
            lastDebounceTime = millis();
        }
        lastReading = reading;

        if (millis() - lastDebounceTime < 50) return;

        if (reading == LOW) {
            if (!wasPressed) {
                wasPressed = true;
                pressedTime = millis();
                lastReportedSec = 0;
                if (Serial) Serial.println("[WifiHelper] Nut FLASH nhan! Giu 5s de reset WiFi...");
                printToLCD("Giu FLASH 5s", "de reset WiFi");
            } else {
                int sec = (millis() - pressedTime) / 1000;
                if (sec > lastReportedSec && sec < 5) {
                    lastReportedSec = sec;
                    if (Serial) Serial.printf("[WifiHelper] Con %ds...\n", 5 - sec);
                    printToLCD("Giu FLASH 5s", String(5 - sec) + "s con lai...");
                }
            }
        } else {
            if (wasPressed) {
                wasPressed = false;
                if (Serial) Serial.println("[WifiHelper] Nha nut som -> bo qua.");
                printToLCD("Da huy", "");
            }
        }

        if (wasPressed && (millis() - pressedTime >= 5000)) {
            if (Serial) Serial.println("[WifiHelper] Reset WiFi! Xoa credentials...");
            printToLCD("Dang xoa WiFi...", "Rebooting...");
            for (int i = 0; i < EEPROM_SIZE; i++) {
                EEPROM.write(i, 0);
            }
            EEPROM.commit();
            WiFi.disconnect(true);
            delay(1000);
            ESP.restart();
        }
    }
};

#endif // WIFI_HELPER_H
