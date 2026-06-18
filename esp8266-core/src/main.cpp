#include <Arduino.h>
#include "Config.h"
#include "WifiHelper.h"
#include "FirebaseHelper.h"
#include "DoorController.h"
#include "RoofController.h"
#include "LightController.h"
#include "ClimateController.h"

// Khởi tạo các đối tượng điều khiển và con trỏ toàn cục cho Stream callback
FirebaseHelper fbHelper;
FirebaseHelper* fbHelperInstance = nullptr;
DoorController doorCtrl;
RoofController roofCtrl;
LightController lightCtrl;
ClimateController climateCtrl;


// Callback xử lý dữ liệu điều khiển từ Firebase stream
void onDeviceControl(const String& device, const String& key, const String& value) {
    #if USE_OPTIMIZED_PINS
        Serial.printf("Command Received: Device: %s, Key: %s, Val: %s\n", device.c_str(), key.c_str(), value.c_str());
    #endif

    if (device == "indoor_light") {
        if (key == "status") {
            lightCtrl.setIndoorLight(value == "true");
        } else if (key == "mode") {
            lightCtrl.setIndoorMode(value);
        } else if (key == "schedule/enabled") {
            lightCtrl.setScheduleEnabled(device, value == "true");
        } else if (key == "schedule/on_time") {
            lightCtrl.setScheduleOnTime(device, value);
        } else if (key == "schedule/off_time") {
            lightCtrl.setScheduleOffTime(device, value);
        }
    } 
    else if (device == "outdoor_light") {
        if (key == "status") {
            lightCtrl.setOutdoorLight(value == "true");
        } else if (key == "mode") {
            lightCtrl.setOutdoorMode(value);
        } else if (key == "schedule/enabled") {
            lightCtrl.setScheduleEnabled(device, value == "true");
        } else if (key == "schedule/on_time") {
            lightCtrl.setScheduleOnTime(device, value);
        } else if (key == "schedule/off_time") {
            lightCtrl.setScheduleOffTime(device, value);
        }
    } 
    else if (device == "door") {
        if (key == "status") {
            doorCtrl.setStatusFromFirebase(value);
        }
    } 
    else if (device == "roof") {
        if (key == "status") {
            roofCtrl.setStatusFromFirebase(value);
        } else if (key == "mode") {
            roofCtrl.setMode(value);
        }
    }
    else if (device == "fan") {
        lightCtrl.setFan(value == "true");
    }
}

void setup() {
    // Chỉ kích hoạt Serial Debug nếu sử dụng cấu hình chân Tối ưu (Tránh xung đột Servo ở RX/TX)
    #if USE_OPTIMIZED_PINS
        Serial.begin(115200);
        delay(500);
        Serial.println("\n======================================");
        Serial.println("SMART HOME SYSTEM BOOTING (OPTIMIZED)");
        Serial.println("======================================");
    #endif

    // 1. Khởi tạo WiFi
    WifiHelper::init();

    // 2. Khởi tạo Firebase
    fbHelper.init(onDeviceControl);

    // 3. Khởi tạo các Mô-đun phần cứng
    lightCtrl.init(&fbHelper);
    doorCtrl.init(&fbHelper);
    roofCtrl.init(&fbHelper);
    climateCtrl.init(&fbHelper);

    // Ghi log hệ thống khởi động thành công
    fbHelper.logEvent("system", "Hệ thống khởi động thành công và đã đồng bộ với Firebase.");

    #if USE_OPTIMIZED_PINS
        Serial.println("System Ready!");
    #endif
}

void loop() {
    // Duy trì các kết nối
    WifiHelper::handleClient();
    WifiHelper::keepAlive();
    fbHelper.keepAlive();

    // Cập nhật hoạt động các mô-đun
    doorCtrl.update();
    roofCtrl.update();
    lightCtrl.update();
    climateCtrl.update();

    // Giải phóng CPU cho ESP8266 chạy background tasks
    yield();
}