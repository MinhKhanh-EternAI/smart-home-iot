#include <Arduino.h>
#include "Config.h"
#include "WifiHelper.h"
#include "FirebaseHelper.h"
#include "DoorController.h"
#include "LightController.h"

// Khởi tạo các thực thể toàn cục
FirebaseHelper fbHelper;
FirebaseHelper* fbHelperInstance = nullptr;

DoorController doorCtrl;
LightController lightCtrl;

// Callback xử lý dữ liệu điều khiển từ Firebase Stream
void onDeviceControl(const String& device, const String& key, const String& value) {
    Serial.printf("[Stream] Nhan update - Thiet bi: %s, Khoa: %s, Gia tri: %s\n", device.c_str(), key.c_str(), value.c_str());
    
    // Xử lý điều khiển Đèn 12V (đồng bộ qua node indoor_light của Firebase)
    if (device == "indoor_light") {
        if (key == "status" || key == "") {
            lightCtrl.setIndoorLight(value == "true");
        }
    }
    // Xử lý thiết lập Cửa
    else if (device == "door") {
        if (key == "auto_close_ms") {
            doorCtrl.setAutoCloseDelay((unsigned long)value.toInt());
        }
    }
}

void setup() {
    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
    delay(500);
    Serial.println("\n==============================================");
    Serial.println("   SMART HOME (AP+STA MODE) SYSTEM BOOTING");
    Serial.println("==============================================");

    // 1. Khởi tạo Wi-Fi và Web Server cục bộ
    WifiHelper::init();

    // Đồng bộ thời gian thực từ máy chủ NTP để xác thực chứng chỉ bảo mật SSL cho Firebase
    if (WiFi.status() == WL_CONNECTED) {
        delay(2000); // Chờ 2 giây để Wi-Fi và DNS ổn định hoàn toàn
        Serial.print("[System] Dang dong bo thoi gian NTP...");
        configTime(7 * 3600, 0, "time.google.com", "time.windows.com");
        time_t now = time(nullptr);
        int retry = 0;
        while (now < 1000000000L && retry < 30) {
            delay(500);
            Serial.print(".");
            now = time(nullptr);
            retry++;
        }
        if (now >= 1000000000L) {
            Serial.println("\n[System] Dong bo thoi gian thanh cong!");
        } else {
            Serial.println("\n[System] Dong bo thoi gian that bai (Timeout).");
        }
    }

    // 2. Gán địa chỉ các đối tượng điều khiển cho WifiHelper sử dụng trong API cục bộ
    WifiHelper::lightCtrl = &lightCtrl;
    WifiHelper::doorCtrl = &doorCtrl;
    WifiHelper::fbHelper = &fbHelper;

    // 3. Khởi tạo Firebase
    fbHelper.init(onDeviceControl);

    // 4. Khởi tạo các Mô-đun phần cứng
    lightCtrl.init(&fbHelper);
    doorCtrl.init(&fbHelper);

    // Đồng bộ trạng thái ban đầu của Đèn lên Firebase
    if (fbHelper.ready()) {
        fbHelper.setBool("/devices/indoor_light/status", lightCtrl.getLightStatus());
        fbHelper.setString("/devices/door/status", doorCtrl.getDoorStatus() ? "open" : "closed");
        fbHelper.logEvent("system", "He thong Smart Home (AP+STA) khoi dong thanh cong.");
    }
    
    Serial.println("[System] Khoi dong hoàn tat. San sang lam viec!");
}

void loop() {
    // Duy trì Web Server và các kết nốli Wi-Fi, Firebase
    WifiHelper::handleClient();
    WifiHelper::keepAlive();
    fbHelper.keepAlive();

    // Cập nhật trạng thái nhịp tim (heartbeat) lên Firebase định kỳ (mỗi 15 giây)
    if (fbHelper.isInitialized() && WiFi.status() == WL_CONNECTED) {
        static unsigned long lastHeartbeatLoop = 0;
        if (lastHeartbeatLoop == 0 || millis() - lastHeartbeatLoop >= 15000) {
            lastHeartbeatLoop = millis();
            fbHelper.updateHeartbeat(WiFi.localIP().toString());
        }
    }

    // Cập nhật trạng thái quét RFID & tự động đóng cửa
    doorCtrl.update();
    
    // Cập nhật đèn
    lightCtrl.update();

    // Nhường CPU cho các tác vụ ngầm của ESP8266
    yield();
}
