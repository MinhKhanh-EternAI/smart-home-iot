#include "WifiHelper.h"
#include "LightController.h"
#include "DoorController.h"
#include "FirebaseHelper.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

// Định nghĩa biến tĩnh cho các đối tượng điều khiển
LightController* WifiHelper::lightCtrl = nullptr;
DoorController* WifiHelper::doorCtrl = nullptr;
FirebaseHelper* WifiHelper::fbHelper = nullptr;

// HTML Dashboard cục bộ lưu trong FLASH (PROGMEM)
const char LOCAL_PORTAL_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Smart Home - Local Portal</title>
    <style>
        :root {
            --primary: #6366f1;
            --primary-hover: #4f46e5;
            --primary-glow: rgba(99, 102, 241, 0.25);
            --bg-gradient: linear-gradient(135deg, #090d16 0%, #111827 50%, #030712 100%);
            --panel-bg: rgba(22, 28, 45, 0.55);
            --panel-border: rgba(255, 255, 255, 0.08);
            --panel-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.5);
            --text-primary: #f9fafb;
            --text-secondary: #9ca3af;
            --active-color: #10b981;
            --inactive-color: #ef4444;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            background: #090d16;
            color: var(--text-primary);
            display: flex;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            margin: 0;
            padding: 16px;
            box-sizing: border-box;
            background-image: var(--bg-gradient);
            background-attachment: fixed;
        }
        .container {
            width: 100%;
            max-width: 440px;
            background: var(--panel-bg);
            border: 1px solid var(--panel-border);
            backdrop-filter: blur(16px);
            -webkit-backdrop-filter: blur(16px);
            border-radius: 20px;
            padding: 24px;
            box-shadow: var(--panel-shadow);
        }
        .header {
            text-align: center;
            margin-bottom: 24px;
        }
        .header h1 {
            font-size: 24px;
            margin: 0 0 6px 0;
            font-weight: 700;
            letter-spacing: -0.5px;
            background: linear-gradient(to right, #ffffff, #cbd5e1);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .header p {
            margin: 0;
            color: var(--text-secondary);
            font-size: 13px;
        }
        .tabs {
            display: flex;
            background: rgba(0, 0, 0, 0.3);
            padding: 4px;
            border-radius: 12px;
            margin-bottom: 24px;
            border: 1px solid rgba(255, 255, 255, 0.05);
        }
        .tab-btn {
            flex: 1;
            padding: 10px;
            background: none;
            border: none;
            color: var(--text-secondary);
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            border-radius: 8px;
            transition: all 0.3s;
        }
        .tab-btn.active {
            background: var(--primary);
            color: white;
            box-shadow: 0 4px 12px var(--primary-glow);
        }
        .tab-content {
            display: none;
        }
        .tab-content.active {
            display: block;
        }
        .card {
            background: rgba(255, 255, 255, 0.02);
            border: 1px solid rgba(255, 255, 255, 0.05);
            border-radius: 14px;
            padding: 16px;
            margin-bottom: 16px;
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
        .card-left {
            display: flex;
            align-items: center;
            gap: 14px;
        }
        .icon-circle {
            width: 44px;
            height: 44px;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            background: rgba(255, 255, 255, 0.04);
            border: 1px solid rgba(255, 255, 255, 0.08);
            transition: all 0.3s;
        }
        .icon-circle svg {
            width: 22px;
            height: 22px;
            fill: var(--text-secondary);
            transition: all 0.3s;
        }
        .card.active .icon-circle {
            background: rgba(99, 102, 241, 0.15);
            border-color: rgba(99, 102, 241, 0.3);
        }
        .card.active .icon-circle svg {
            fill: #818cf8;
            filter: drop-shadow(0 0 4px #818cf8);
        }
        .card-details h3 {
            margin: 0;
            font-size: 15px;
            font-weight: 600;
        }
        .card-details span {
            font-size: 12px;
            color: var(--text-secondary);
        }
        /* iOS-style toggle switch */
        .switch {
            position: relative;
            display: inline-block;
            width: 46px;
            height: 26px;
        }
        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: rgba(255, 255, 255, 0.15);
            transition: .4s;
            border-radius: 34px;
            border: 1px solid rgba(255, 255, 255, 0.05);
        }
        .slider:before {
            position: absolute;
            content: "";
            height: 20px;
            width: 20px;
            left: 2px;
            bottom: 2px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
            box-shadow: 0 2px 4px rgba(0,0,0,0.4);
        }
        input:checked + .slider {
            background-color: var(--active-color);
        }
        input:checked + .slider:before {
            transform: translateX(20px);
        }
        /* Open Door Button */
        .btn-action {
            background: var(--primary);
            color: white;
            border: none;
            padding: 10px 18px;
            border-radius: 10px;
            font-size: 13px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
            box-shadow: 0 4px 12px var(--primary-glow);
        }
        .btn-action:hover {
            background: var(--primary-hover);
            transform: translateY(-1px);
        }
        .btn-action:active {
            transform: translateY(0);
        }
        .status-pill-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
            margin-top: 24px;
        }
        .status-pill {
            background: rgba(0, 0, 0, 0.25);
            border: 1px solid rgba(255, 255, 255, 0.05);
            border-radius: 10px;
            padding: 10px 12px;
            text-align: center;
        }
        .status-pill .title {
            display: block;
            font-size: 11px;
            color: var(--text-secondary);
            margin-bottom: 4px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .status-pill .value {
            font-size: 13px;
            font-weight: 600;
        }
        .dot {
            display: inline-block;
            width: 8px;
            height: 8px;
            border-radius: 50%;
            margin-right: 6px;
        }
        .dot.online { background-color: var(--active-color); box-shadow: 0 0 8px var(--active-color); }
        .dot.offline { background-color: var(--inactive-color); box-shadow: 0 0 8px var(--inactive-color); }
        
        /* Tab Config Styles */
        input, select {
            width: 100%;
            padding: 12px;
            background: rgba(0, 0, 0, 0.25);
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 10px;
            color: white;
            font-size: 14px;
            margin-bottom: 16px;
            box-sizing: border-box;
            outline: none;
            transition: all 0.3s;
        }
        input:focus, select:focus {
            border-color: var(--primary);
            box-shadow: 0 0 0 3px var(--primary-glow);
        }
        .btn-submit {
            background: var(--primary);
            color: white;
            border: none;
            padding: 12px;
            border-radius: 10px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            width: 100%;
            transition: all 0.3s;
            box-shadow: 0 4px 14px var(--primary-glow);
        }
        .btn-submit:hover {
            background: var(--primary-hover);
        }
        .btn-scan {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.08);
            color: white;
            font-size: 13px;
            padding: 8px 12px;
            border-radius: 8px;
            cursor: pointer;
            margin-bottom: 16px;
            transition: all 0.3s;
        }
        .btn-scan:hover {
            background: rgba(255,255,255,0.1);
        }
        .success-banner {
            display: none;
            background: rgba(16, 185, 129, 0.1);
            border: 1px solid rgba(16, 185, 129, 0.2);
            color: #34d399;
            border-radius: 10px;
            padding: 12px;
            font-size: 13px;
            text-align: center;
            margin-top: 16px;
            line-height: 1.4;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Smart Home</h1>
            <p>Local Control & Settings Portal</p>
        </div>

        <div class="tabs">
            <button class="tab-btn active" onclick="switchTab('control')">Điều khiển</button>
            <button class="tab-btn" onclick="switchTab('wifi')">Wi-Fi Cấu hình</button>
        </div>

        <!-- TAB 1: CONTROL PANEL -->
        <div id="tab-control" class="tab-content active">
            <!-- Light Card -->
            <div class="card" id="light-card">
                <div class="card-left">
                    <div class="icon-circle">
                        <!-- Bulb SVG Icon -->
                        <svg viewBox="0 0 24 24"><path d="M12 2C7.58 2 4 5.58 4 10c0 2.22.9 4.22 2.34 5.67L8 17.34V20c0 .55.45 1 1 1h6c.55 0 1-.45 1-1v-2.66l1.66-1.67C19.1 14.22 20 12.22 20 10c0-4.42-3.58-8-8-8zm-1 17h2v1h-2v-1zm4-3H9v-1h6v1zm.66-2.34l-.66.68V15H9v-.66l-.66-.68A5.94 5.94 0 0 1 6 10c0-3.31 2.69-6 6-6s6 2.69 6 6c0 1.91-.9 3.63-2.34 4.66z"/></svg>
                    </div>
                    <div class="card-details">
                        <h3>Đèn 12V Relay</h3>
                        <span id="light-status-lbl">Trạng thái: Đang tắt</span>
                    </div>
                </div>
                <label class="switch">
                    <input type="checkbox" id="light-toggle" onchange="toggleLight(this.checked)">
                    <span class="slider"></span>
                </label>
            </div>

            <!-- Door Card -->
            <div class="card" id="door-card">
                <div class="card-left">
                    <div class="icon-circle">
                        <!-- Key/Lock SVG Icon -->
                        <svg viewBox="0 0 24 24"><path d="M18 8h-1V6c0-2.76-2.24-5-5-5S7 3.24 7 6v2H6c-1.1 0-2 .9-2 2v10c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V10c0-1.1-.9-2-2-2zm-6 9c-1.1 0-2-.9-2-2s.9-2 2-2 2 .9 2 2-.9 2-2 2zm3.1-9H8.9V6c0-1.71 1.39-3.1 3.1-3.1 1.71 0 3.1 1.39 3.1 3.1v2z"/></svg>
                    </div>
                    <div class="card-details">
                        <h3>Cửa thông minh</h3>
                        <span id="door-status-lbl">Trạng thái: Đóng</span>
                    </div>
                </div>
                <button class="btn-action" onclick="openDoor()">Mở Cửa</button>
            </div>

            <!-- Status Pills -->
            <div class="status-pill-grid">
                <div class="status-pill">
                    <span class="title">Wi-Fi Router</span>
                    <span class="value" id="wifi-ssid-lbl">Checking...</span>
                </div>
                <div class="status-pill">
                    <span class="title">Firebase Sync</span>
                    <span class="value"><span class="dot" id="fb-dot"></span><span id="fb-lbl">Checking...</span></span>
                </div>
            </div>
        </div>

        <!-- TAB 2: WI-FI SETTINGS -->
        <div id="tab-wifi" class="tab-content">
            <button class="btn-scan" id="btn-scan" onclick="scanNetworks()">Quét mạng Wi-Fi xung quanh</button>
            <select id="ssid-select" style="display: none;" onchange="document.getElementById('ssid-input').value = this.value">
                <option value="">-- Chọn mạng Wi-Fi --</option>
            </select>
            <input type="text" id="ssid-input" placeholder="Tên Wi-Fi (SSID)">
            <input type="password" id="password" placeholder="Mật khẩu Wi-Fi">
            
            <button class="btn-submit" onclick="saveConfig()">Lưu cấu hình & Kết nối</button>

            <div class="success-banner" id="success">
                <b>Đã lưu cấu hình thành công!</b><br>
                ESP8266 đang khởi động lại để kết nối vào Wi-Fi mới...
            </div>
        </div>
    </div>

    <script>
        function switchTab(tabName) {
            document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
            
            const activeBtn = Array.from(document.querySelectorAll('.tab-btn')).find(btn => btn.innerText.toLowerCase().includes(tabName === 'control' ? 'điều' : 'wi-fi'));
            if(activeBtn) activeBtn.classList.add('active');
            
            document.getElementById('tab-' + tabName).classList.add('active');
        }

        // Đọc trạng thái định kỳ từ ESP8266
        function getStatus() {
            fetch('/api/status')
                .then(res => res.json())
                .then(data => {
                    // Cập nhật đèn
                    const lightCard = document.getElementById('light-card');
                    const lightToggle = document.getElementById('light-toggle');
                    const lightLbl = document.getElementById('light-status-lbl');
                    lightToggle.checked = data.light;
                    lightLbl.innerText = 'Trạng thái: ' + (data.light ? 'Đang bật' : 'Đang tắt');
                    lightCard.classList.toggle('active', data.light);

                    // Cập nhật cửa
                    const doorCard = document.getElementById('door-card');
                    const doorLbl = document.getElementById('door-status-lbl');
                    const isOpen = (data.door === 'open');
                    doorLbl.innerText = 'Trạng thái: ' + (isOpen ? 'MỞ CỬA' : 'Đóng');
                    doorCard.classList.toggle('active', isOpen);

                    // Cập nhật Wi-Fi
                    const ssidLbl = document.getElementById('wifi-ssid-lbl');
                    if(data.wifi_status === 'connected') {
                        ssidLbl.innerText = data.ssid;
                        ssidLbl.style.color = '#34d399';
                    } else {
                        ssidLbl.innerText = 'Disconnected';
                        ssidLbl.style.color = '#f87171';
                    }

                    // Cập nhật Firebase
                    const fbDot = document.getElementById('fb-dot');
                    const fbLbl = document.getElementById('fb-lbl');
                    if(data.firebase_status === 'ready') {
                        fbDot.className = 'dot online';
                        fbLbl.innerText = 'Online';
                    } else {
                        fbDot.className = 'dot offline';
                        fbLbl.innerText = 'Offline';
                    }
                })
                .catch(err => console.error("Lỗi đọc status:", err));
        }

        function toggleLight(state) {
            const formData = new FormData();
            formData.append('state', state ? '1' : '0');
            fetch('/api/light', { method: 'POST', body: formData })
                .then(res => res.json())
                .then(data => {
                    getStatus();
                });
        }

        function openDoor() {
            const formData = new FormData();
            formData.append('action', 'open');
            fetch('/api/door', { method: 'POST', body: formData })
                .then(res => res.json())
                .then(data => {
                    getStatus();
                });
        }

        function scanNetworks() {
            const btn = document.getElementById('btn-scan');
            const select = document.getElementById('ssid-select');
            btn.disabled = true;
            btn.innerText = 'Đang quét WiFi...';
            
            fetch('/scan')
                .then(res => res.json())
                .then(data => {
                    select.innerHTML = '<option value="">-- Chọn mạng Wi-Fi --</option>';
                    if(data && data.length > 0) {
                        data.forEach(net => {
                            const opt = document.createElement('option');
                            opt.value = net.ssid;
                            opt.innerText = net.ssid + ' (' + net.rssi + ' dBm)';
                            select.appendChild(opt);
                        });
                        select.style.display = 'block';
                    } else {
                        alert('Không tìm thấy mạng Wi-Fi nào xung quanh!');
                    }
                })
                .catch(err => alert('Lỗi khi quét Wi-Fi!'))
                .finally(() => {
                    btn.disabled = false;
                    btn.innerText = 'Quét mạng Wi-Fi xung quanh';
                });
        }

        function saveConfig() {
            const ssid = document.getElementById('ssid-input').value.trim();
            const pass = document.getElementById('password').value;
            if(!ssid) {
                alert('Vui lòng chọn hoặc nhập SSID Wi-Fi!');
                return;
            }
            const formData = new FormData();
            formData.append('ssid', ssid);
            formData.append('password', pass);
            
            fetch('/save', { method: 'POST', body: formData })
                .then(res => {
                    document.getElementById('success').style.display = 'block';
                })
                .catch(err => alert('Lỗi khi lưu cấu hình Wi-Fi!'));
        }

        // Định kỳ lấy trạng thái mỗi 2 giây
        setInterval(getStatus, 2000);
        getStatus();
    </script>
</body>
</html>
)rawhtml";

void WifiHelper::printToLCD(const String& line1, const String& line2) {
    int addr = getLCDAddress();
    if (addr != -1) {
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

void WifiHelper::init() {
    Serial.println("\n--- [WifiHelper] Bat dau khoi tao WiFi & EEPROM ---");
    EEPROM.begin(EEPROM_SIZE);
    Serial.println("[WifiHelper] EEPROM.begin() khoi tao thanh cong.");
    
    String savedSSID = readStringFromEEPROM(0, 32);
    String savedPass = readStringFromEEPROM(32, 64);
    
    Serial.printf("[WifiHelper] SSID doc tu EEPROM: \"%s\"\n", savedSSID.c_str());
    Serial.printf("[WifiHelper] Pass doc tu EEPROM: \"%s\"\n", savedPass.c_str());
    
    Serial.printf("[WifiHelper] Khoi dong I2C Bus... SDA Pin: %d, SCL Pin: %d\n", I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
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
        
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(savedSSID.c_str(), savedPass.c_str());
        
        Serial.printf("[WifiHelper] Dang ket noi Wi-Fi: %s ", savedSSID.c_str());
        
        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 30) {
            delay(500);
            Serial.print(".");
            timeout++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            Serial.println("\n[WifiHelper] Wi-Fi da ket noi thanh cong!");
            Serial.print("[WifiHelper] IP cua ESP8266: "); Serial.println(WiFi.localIP());
            printToLCD("WiFi Connected!", WiFi.localIP().toString());
            delay(2000);
        } else {
            Serial.println("\n[WifiHelper] Ket noi Wi-Fi that bai (Timeout)!");
        }
    } else {
        Serial.println("[WifiHelper] Khong co thong tin WiFi hop le hoac day la lan khoi dong dau.");
    }
    
    startAP();
}

void WifiHelper::startAP() {
    WiFi.mode(WIFI_AP_STA);
    
    IPAddress apIP(AP_IP_ADDR);
    IPAddress gateway(AP_IP_ADDR);
    IPAddress subnet(AP_SUBNET_ADDR);
    WiFi.softAPConfig(apIP, gateway, subnet);
    
    WiFi.softAP(AP_SSID, AP_PASS);
    
    Serial.printf("[WifiHelper] Mang Wi-Fi phat ra: %s (Pass: %s)\n", AP_SSID, AP_PASS);
    Serial.print("[WifiHelper] Dia chi IP truy cap cuc bo: "); Serial.println(WiFi.softAPIP());

    ESP8266WebServer& server = getServer();

    // 1. Giao diện portal
    server.on("/", HTTP_GET, []() {
        getServer().send_P(200, "text/html", LOCAL_PORTAL_HTML);
    });

    // 2. Lấy trạng thái thiết bị
    server.on("/api/status", HTTP_GET, []() {
        bool lightState = (lightCtrl != nullptr) ? lightCtrl->getLightStatus() : false;
        String doorState = (doorCtrl != nullptr && doorCtrl->getDoorStatus()) ? "open" : "closed";
        
        String json = "{";
        json += "\"light\":" + String(lightState ? "true" : "false") + ",";
        json += "\"door\":\"" + doorState + "\",";
        json += "\"ssid\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "Not Connected") + "\",";
        json += "\"wifi_status\":\"" + String(WiFi.status() == WL_CONNECTED ? "connected" : "disconnected") + "\",";
        json += "\"firebase_status\":\"" + String(fbHelper != nullptr && fbHelper->ready() ? "ready" : "not_ready") + "\"";
        json += "}";
        
        getServer().send(200, "application/json", json);
    });

    // 3. Điều khiển đèn 12V
    server.on("/api/light", HTTP_POST, []() {
        String stateArg = getServer().arg("state");
        if (stateArg.length() > 0) {
            bool state = (stateArg == "1" || stateArg == "true");
            if (lightCtrl != nullptr) {
                lightCtrl->setIndoorLight(state);
            }
            getServer().send(200, "application/json", "{\"status\":\"ok\",\"light\":" + String(state ? "true" : "false") + "}");
        } else {
            getServer().send(400, "application/json", "{\"error\":\"Missing parameter: state\"}");
        }
    });

    // 4. Điều khiển mở cửa
    server.on("/api/door", HTTP_POST, []() {
        String actionArg = getServer().arg("action");
        if (actionArg == "open") {
            if (doorCtrl != nullptr) {
                doorCtrl->openDoor();
            }
            getServer().send(200, "application/json", "{\"status\":\"ok\",\"door\":\"open\"}");
        } else {
            getServer().send(400, "application/json", "{\"error\":\"Missing or invalid parameter: action\"}");
        }
    });

    // 5. Quét WiFi
    server.on("/scan", HTTP_GET, []() {
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n; ++i) {
            if (i > 0) json += ",";
            json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";
        getServer().send(200, "application/json", json);
    });

    // 6. Lưu cấu hình WiFi
    server.on("/save", HTTP_POST, []() {
        String ssid = getServer().arg("ssid");
        String password = getServer().arg("password");
        
        writeStringToEEPROM(0, ssid);
        writeStringToEEPROM(32, password);
        
        getServer().send(200, "application/json", "{\"status\":\"ok\"}");
        printToLCD("Config Saved", "Rebooting...");
        
        delay(1500);
        ESP.restart();
    });

    server.begin();
    Serial.println("[WifiHelper] Web Server da khoi dong thanh cong tren Port 80.");
}

void WifiHelper::handleClient() {
    getServer().handleClient();
}

void WifiHelper::keepAlive() {
    if (WiFi.getMode() == WIFI_AP_STA && WiFi.status() != WL_CONNECTED) {
        static unsigned long lastReconnectAttempt = 0;
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 20000) {
            lastReconnectAttempt = now;
            Serial.println("[WifiHelper] Wi-Fi bi ngat. Dang ket noi lai...");
            
            String savedSSID = readStringFromEEPROM(0, 32);
            String savedPass = readStringFromEEPROM(32, 64);
            
            if (savedSSID.length() > 0 && savedSSID != "Your_SSID" && savedSSID[0] != 0xFF) {
                WiFi.begin(savedSSID.c_str(), savedPass.c_str());
            }
        }
    }
}
