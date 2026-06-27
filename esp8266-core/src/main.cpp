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
        } else if (key.startsWith("schedule/days/")) {
            String dayName = key.substring(14);
            lightCtrl.setScheduleDay(device, dayName, value == "true");

        } else if (key == "schedule") {
            FirebaseJson json;
            json.setJsonData(value);
            FirebaseJsonData result;
            
            json.get(result, "enabled");
            if (result.success) lightCtrl.setScheduleEnabled(device, result.to<bool>());
            
            json.get(result, "on_time");
            if (result.success) lightCtrl.setScheduleOnTime(device, result.to<String>());
            
            json.get(result, "off_time");
            if (result.success) lightCtrl.setScheduleOffTime(device, result.to<String>());
            
            json.get(result, "days");
            if (result.success) {
                FirebaseJson daysJson;
                result.getJSON(daysJson);
                if (daysJson.iteratorBegin() > 0) {
                    FirebaseJsonData dayResult;
                    const char* dayNames[7] = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};
                    for (int i = 0; i < 7; i++) {
                        daysJson.get(dayResult, dayNames[i]);
                        if (dayResult.success) {
                            lightCtrl.setScheduleDay(device, dayNames[i], dayResult.to<bool>());
                        }
                    }
                    daysJson.iteratorEnd();
                }
            }

        }
    }
    else if (device == "outdoor_light") {
        if (key == "status") {
            lightCtrl.setOutdoorLight(value == "true");
        } else if (key == "mode") {
            lightCtrl.setOutdoorMode(value);
        }
    } 
    else if (device == "door") {
        if (key == "auto_close_ms") {
            doorCtrl.setAutoCloseDelay((unsigned long)value.toInt());
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
        if (key == "status" || key == "") {
            lightCtrl.setFan(value == "true");
        } else if (key == "schedule/enabled") {
            lightCtrl.setScheduleEnabled(device, value == "true");
        } else if (key == "schedule/on_time") {
            lightCtrl.setScheduleOnTime(device, value);
        } else if (key == "schedule/off_time") {
            lightCtrl.setScheduleOffTime(device, value);
        } else if (key.startsWith("schedule/days/")) {
            String dayName = key.substring(14);
            lightCtrl.setScheduleDay(device, dayName, value == "true");
        } else if (key == "schedule") {
            FirebaseJson json;
            json.setJsonData(value);
            FirebaseJsonData result;
            
            json.get(result, "enabled");
            if (result.success) lightCtrl.setScheduleEnabled(device, result.to<bool>());
            
            json.get(result, "on_time");
            if (result.success) lightCtrl.setScheduleOnTime(device, result.to<String>());
            
            json.get(result, "off_time");
            if (result.success) lightCtrl.setScheduleOffTime(device, result.to<String>());
            
            json.get(result, "days");
            if (result.success) {
                FirebaseJson daysJson;
                result.getJSON(daysJson);
                if (daysJson.iteratorBegin() > 0) {
                    FirebaseJsonData dayResult;
                    const char* dayNames[7] = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};
                    for (int i = 0; i < 7; i++) {
                        daysJson.get(dayResult, dayNames[i]);
                        if (dayResult.success) {
                            lightCtrl.setScheduleDay(device, dayNames[i], dayResult.to<bool>());
                        }
                    }
                    daysJson.iteratorEnd();
                }
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n================================");
    Serial.println("SMART HOME SYSTEM BOOTING");
    Serial.println("================================");

    // 1. Khởi tạo WiFi
    WifiHelper::init();

    // 2. Khởi tạo Firebase
    fbHelper.init(onDeviceControl);

    // 3. Khởi tạo các Mô-đun phần cứng
    lightCtrl.init(&fbHelper);
    doorCtrl.init(&fbHelper);
    climateCtrl.init(&fbHelper);

    Serial.println("Serial closing — RX (GPIO3) freed for Servo Roof.");
    Serial.flush();
    Serial.end();

    roofCtrl.init(&fbHelper);

    fbHelper.logEvent("system", "Hệ thống khởi động thành công và đã đồng bộ với Firebase.");
}

void loop() {
    // Duy trì các kết nối
    WifiHelper::handleClient();
    WifiHelper::keepAlive();
    fbHelper.keepAlive();

    // Cập nhật trạng thái nhịp tim (heartbeat) & IP cục bộ lên Firebase
    if (WiFi.status() == WL_CONNECTED) {
        fbHelper.updateHeartbeat(WiFi.localIP().toString());
    }

    // Cập nhật hoạt động các mô-đun
    doorCtrl.update();
    roofCtrl.update();
    lightCtrl.update();
    climateCtrl.update();

    // Giải phóng CPU cho ESP8266 chạy background tasks
    yield();
}