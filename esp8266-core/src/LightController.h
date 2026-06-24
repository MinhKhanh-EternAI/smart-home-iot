#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include <PCF8574.h>
#include <time.h>
#include "Config.h"
#include "FirebaseHelper.h"

class LightController {
private:
    PCF8574 pcf;
    FirebaseHelper* fb;
    
    bool indoorStatus = false;
    bool outdoorStatus = false;
    bool fanStatus = false;
    
    String indoorMode = "manual"; // "manual" hoặc "schedule"
    String outdoorMode = "auto"; // "auto" hoặc "manual"
    bool pirState = false;
    unsigned long lastMotionTime = 0;
    const unsigned long motionTimeout = 30000; // Tắt đèn sau 30 giây không phát hiện chuyển động

    // Quản lý hẹn giờ
    bool indoorScheduleEnabled = false;
    int indoorOnHour = -1, indoorOnMin = -1;
    int indoorOffHour = -1, indoorOffMin = -1;
    int lastIndoorTriggerHour = -1, lastIndoorTriggerMin = -1;
    int indoorScheduleDays = 127; // Bitmask: bit0=CN, bit1=T2,...,bit6=T7, 127=mọi ngày

    bool outdoorScheduleEnabled = false;
    int outdoorOnHour = -1, outdoorOnMin = -1;
    int outdoorOffHour = -1, outdoorOffMin = -1;
    int lastOutdoorTriggerHour = -1, lastOutdoorTriggerMin = -1;
    int outdoorScheduleDays = 127;

    bool fanScheduleEnabled = false;
    int fanOnHour = -1, fanOnMin = -1;
    int fanOffHour = -1, fanOffMin = -1;
    int lastFanTriggerHour = -1, lastFanTriggerMin = -1;
    int fanScheduleDays = 127;
    
    unsigned long lastScheduleCheck = 0;

    static int dayNameToBit(const String& name) {
        if (name == "monday") return 1;
        if (name == "tuesday") return 2;
        if (name == "wednesday") return 3;
        if (name == "thursday") return 4;
        if (name == "friday") return 5;
        if (name == "saturday") return 6;
        if (name == "sunday") return 0;
        return -1;
    }

public:
    LightController() : pcf(PCF_I2C_ADDRESS) {}

    void init(FirebaseHelper* fbHelper) {
        fb = fbHelper;
        
        // I2C bus đã được WifiHelper::init() khởi tạo — không gọi Wire.begin() lại
        pcf.begin();
        
        // Trạng thái ban đầu: Tắt hết thiết bị (Relay ở mức cao - HIGH vì là Active Low)
        writeRelay(PCF_RELAY_OUTDOOR, false);
        writeRelay(PCF_RELAY_INDOOR, false);
        writeRelay(PCF_BUZZER, false);
        writeRelay(PCF_FAN_RELAY, false);

        // Cảm biến vật cản hồng ngoại LM393 trên PCF8574 P3 (quasi-bidirectional, ghi HIGH trước khi đọc)
        pcf.write(PCF_LM393_INPUT, HIGH);
        Serial.println("LM393 IR Sensor configured on PCF8574 Pin 3");


        // Khởi tạo thời gian hệ thống qua NTP (Múi giờ GMT+7 của Việt Nam)
        configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
        Serial.println("NTP Time Sync initialized.");
    }

    void update() {
        // 1. Đọc cảm biến vật cản hồng ngoại LM393 (LOW = có vật cản)
        bool currentPIR = (pcf.read(PCF_LM393_INPUT) == LOW);

        if (currentPIR != pirState) {
            pirState = currentPIR;
            fb->setBool("/sensors/motion", pirState);
            
            if (pirState) {
                lastMotionTime = millis();
                Serial.println("Motion detected!");
                if (outdoorMode == "auto" && !outdoorStatus) {
                    setOutdoorLight(true);
                    fb->logEvent("automation", "Tự động bật đèn cửa do phát hiện người.");
                }
            } else {
                Serial.println("Motion ended.");
            }
        }

        // 2. Tắt tự động đèn ngoài trời sau khi hết chuyển động (chế độ auto)
        if (outdoorMode == "auto" && outdoorStatus && !pirState) {
            if (millis() - lastMotionTime >= motionTimeout) {
                setOutdoorLight(false);
                fb->logEvent("automation", "Tự động tắt đèn cửa sau 30 giây không phát hiện người.");
            }
        }

        // 3. Kiểm tra hẹn giờ định kỳ mỗi 5 giây
        unsigned long now = millis();
        if (now - lastScheduleCheck >= 5000) {
            lastScheduleCheck = now;
            checkSchedules();
        }
    }

    void setIndoorLight(bool status) {
        indoorStatus = status;
        writeRelay(PCF_RELAY_INDOOR, indoorStatus);
        fb->setBool("/devices/indoor_light/status", indoorStatus);
        Serial.printf("Indoor Light status: %s\n", indoorStatus ? "ON" : "OFF");
    }

    void setOutdoorLight(bool status) {
        outdoorStatus = status;
        writeRelay(PCF_RELAY_OUTDOOR, outdoorStatus);
        fb->setBool("/devices/outdoor_light/status", outdoorStatus);
        Serial.printf("Outdoor Light status: %s\n", outdoorStatus ? "ON" : "OFF");
    }

    void setFan(bool status) {
        fanStatus = status;
        writeRelay(PCF_FAN_RELAY, fanStatus);
        fb->setBool("/devices/fan", fanStatus);
        Serial.printf("Fan status: %s\n", fanStatus ? "ON" : "OFF");
    }

    void setIndoorMode(const String& mode) {
        if (mode == "schedule" || mode == "manual") {
            indoorMode = mode;
            fb->setString("/devices/indoor_light/mode", indoorMode);
            Serial.printf("Indoor Light mode updated: %s\n", indoorMode.c_str());
        }
    }

    void setOutdoorMode(const String& mode) {
        if (mode == "auto" || mode == "manual") {
            outdoorMode = mode;
            fb->setString("/devices/outdoor_light/mode", outdoorMode);
            Serial.printf("Outdoor Light mode updated: %s\n", outdoorMode.c_str());
        }
    }

    // Thiết lập từng thuộc tính hẹn giờ từ Firebase stream
    void setScheduleEnabled(const String& device, bool enabled) {
        if (device == "indoor_light") {
            indoorScheduleEnabled = enabled;
        } else if (device == "outdoor_light") {
            outdoorScheduleEnabled = enabled;
        } else if (device == "fan") {
            fanScheduleEnabled = enabled;
        }
        Serial.printf("Schedule for %s: %s\n", device.c_str(), enabled ? "ENABLED" : "DISABLED");
    }

    void setScheduleOnTime(const String& device, const String& onTime) {
        int hour = -1, min = -1;
        if (sscanf(onTime.c_str(), "%d:%d", &hour, &min) == 2) {
            if (device == "indoor_light") {
                indoorOnHour = hour;
                indoorOnMin = min;
            } else if (device == "outdoor_light") {
                outdoorOnHour = hour;
                outdoorOnMin = min;
            } else if (device == "fan") {
                fanOnHour = hour;
                fanOnMin = min;
            }
            Serial.printf("Schedule ON for %s: %02d:%02d\n", device.c_str(), hour, min);
        }
    }

    void setScheduleDays(const String& device, int days) {
        if (device == "indoor_light") indoorScheduleDays = days;
        else if (device == "outdoor_light") outdoorScheduleDays = days;
        else if (device == "fan") fanScheduleDays = days;
        Serial.printf("Schedule days for %s: %d\n", device.c_str(), days);
    }

    void setScheduleDay(const String& device, const String& dayName, bool enabled) {
        int bit = dayNameToBit(dayName);
        if (bit < 0) return;
        int& days = (device == "indoor_light") ? indoorScheduleDays : 
                    (device == "fan") ? fanScheduleDays : outdoorScheduleDays;
        if (enabled) days |= (1 << bit);
        else days &= ~(1 << bit);
        Serial.printf("Schedule day %s for %s: %s (bitmask: %d)\n", dayName.c_str(), device.c_str(), enabled ? "ON" : "OFF", days);
    }

    void setScheduleOffTime(const String& device, const String& offTime) {
        int hour = -1, min = -1;
        if (sscanf(offTime.c_str(), "%d:%d", &hour, &min) == 2) {
            if (device == "indoor_light") {
                indoorOffHour = hour;
                indoorOffMin = min;
            } else if (device == "outdoor_light") {
                outdoorOffHour = hour;
                outdoorOffMin = min;
            } else if (device == "fan") {
                fanOffHour = hour;
                fanOffMin = min;
            }
            Serial.printf("Schedule OFF for %s: %02d:%02d\n", device.c_str(), hour, min);
        }
    }


private:
    // Hỗ trợ bật/tắt Relay (Giả sử Relay Kích ở mức THẤP - Active Low)
    void writeRelay(uint8_t pin, bool turnOn) {
        // Nếu active low, bật (turnOn=true) -> ghi LOW, tắt -> ghi HIGH
        pcf.write(pin, turnOn ? LOW : HIGH);
    }

    void checkSchedules() {
        time_t rawtime;
        struct tm* timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        if (timeinfo->tm_year < 120) {
            // Chưa đồng bộ được thời gian NTP (năm < 2020)
            return;
        }

        int currentHour = timeinfo->tm_hour;
        int currentMin = timeinfo->tm_min;
        int currentDow = timeinfo->tm_wday; // 0=CN, 1=T2, ..., 6=T7

        // Kiểm tra đèn trong nhà (chỉ chạy khi đã kích hoạt lịch trình và đúng ngày)
        if (indoorScheduleEnabled && indoorOnHour != -1 && indoorOffHour != -1
            && (indoorScheduleDays & (1 << currentDow))) {
            if (currentHour == indoorOnHour && currentMin == indoorOnMin) {
                if (currentHour != lastIndoorTriggerHour || currentMin != lastIndoorTriggerMin) {
                    lastIndoorTriggerHour = currentHour;
                    lastIndoorTriggerMin = currentMin;
                    if (!indoorStatus) {
                        setIndoorLight(true);
                        fb->logEvent("schedule", "Hẹn giờ: Bật đèn trong nhà.");
                    }
                }
            } else if (currentHour == indoorOffHour && currentMin == indoorOffMin) {
                if (currentHour != lastIndoorTriggerHour || currentMin != lastIndoorTriggerMin) {
                    lastIndoorTriggerHour = currentHour;
                    lastIndoorTriggerMin = currentMin;
                    if (indoorStatus) {
                        setIndoorLight(false);
                        fb->logEvent("schedule", "Hẹn giờ: Tắt đèn trong nhà.");
                    }
                }
            }
        }

        // Kiểm tra đèn ngoài trời (chỉ chạy khi ở chế độ Thủ công và đúng ngày)
        if (outdoorMode == "manual" && outdoorScheduleEnabled && outdoorOnHour != -1 && outdoorOffHour != -1
            && (outdoorScheduleDays & (1 << currentDow))) {
            if (currentHour == outdoorOnHour && currentMin == outdoorOnMin) {
                if (currentHour != lastOutdoorTriggerHour || currentMin != lastOutdoorTriggerMin) {
                    lastOutdoorTriggerHour = currentHour;
                    lastOutdoorTriggerMin = currentMin;
                    if (!outdoorStatus) {
                        setOutdoorLight(true);
                        fb->logEvent("schedule", "Hẹn giờ: Bật đèn ngoài trời.");
                    }
                }
            } else if (currentHour == outdoorOffHour && currentMin == outdoorOffMin) {
                if (currentHour != lastOutdoorTriggerHour || currentMin != lastOutdoorTriggerMin) {
                    lastOutdoorTriggerHour = currentHour;
                    lastOutdoorTriggerMin = currentMin;
                    if (outdoorStatus) {
                        setOutdoorLight(false);
                        fb->logEvent("schedule", "Hẹn giờ: Tắt đèn ngoài trời.");
                    }
                }
            }
        }

        // Kiểm tra quạt (chỉ chạy khi đã kích hoạt lịch trình và đúng ngày)
        if (fanScheduleEnabled && fanOnHour != -1 && fanOffHour != -1
            && (fanScheduleDays & (1 << currentDow))) {
            if (currentHour == fanOnHour && currentMin == fanOnMin) {
                if (currentHour != lastFanTriggerHour || currentMin != lastFanTriggerMin) {
                    lastFanTriggerHour = currentHour;
                    lastFanTriggerMin = currentMin;
                    if (!fanStatus) {
                        setFan(true);
                        fb->logEvent("schedule", "Hẹn giờ: Bật quạt.");
                    }
                }
            } else if (currentHour == fanOffHour && currentMin == fanOffMin) {
                if (currentHour != lastFanTriggerHour || currentMin != lastFanTriggerMin) {
                    lastFanTriggerHour = currentHour;
                    lastFanTriggerMin = currentMin;
                    if (fanStatus) {
                        setFan(false);
                        fb->logEvent("schedule", "Hẹn giờ: Tắt quạt.");
                    }
                }
            }
        }
    }
};

#endif // LIGHT_CONTROLLER_H
