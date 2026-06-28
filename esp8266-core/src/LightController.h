#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include <PCF8574.h>
#include <time.h>
#include "Config.h"
#include "FirebaseHelper.h"
#include "WifiHelper.h"

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

            lastMotionTime = millis();

            if (pirState) {
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
        if (status == indoorStatus) return;
        indoorStatus = status;
        writeRelay(PCF_RELAY_INDOOR, indoorStatus);
        fb->setBool("/devices/indoor_light/status", indoorStatus);
        Serial.printf("Indoor Light status: %s\n", indoorStatus ? "ON" : "OFF");
        WifiHelper::showNotification("Den trong nha", status ? "BAT" : "TAT");
    }

    void setOutdoorLight(bool status) {
        if (status == outdoorStatus) return;
        outdoorStatus = status;
        writeRelay(PCF_RELAY_OUTDOOR, outdoorStatus);
        fb->setBool("/devices/outdoor_light/status", outdoorStatus);
        Serial.printf("Outdoor Light status: %s\n", outdoorStatus ? "ON" : "OFF");
        WifiHelper::showNotification("Den ngoai", status ? "BAT" : "TAT");
    }

    void setFan(bool status) {
        if (status == fanStatus) return;
        fanStatus = status;
        writeRelay(PCF_FAN_RELAY, fanStatus);
        fb->setBool("/devices/fan/status", fanStatus);
        Serial.printf("Fan status: %s\n", fanStatus ? "ON" : "OFF");
        WifiHelper::showNotification("Quat", status ? "BAT" : "TAT");
    }

    void setIndoorMode(const String& mode) {
        if (mode != "schedule" && mode != "manual") return;
        if (mode == indoorMode) return;
        indoorMode = mode;
        fb->setString("/devices/indoor_light/mode", indoorMode);
        Serial.printf("Indoor Light mode updated: %s\n", indoorMode.c_str());
    }

    void setOutdoorMode(const String& mode) {
        if (mode != "auto" && mode != "manual") return;
        if (mode == outdoorMode) return;
        outdoorMode = mode;
        fb->setString("/devices/outdoor_light/mode", outdoorMode);
        Serial.printf("Outdoor Light mode updated: %s\n", outdoorMode.c_str());
    }

    void setScheduleEnabled(const String& device, bool enabled) {
        if (device == "indoor_light") indoorScheduleEnabled = enabled;
        else if (device == "fan") fanScheduleEnabled = enabled;
        Serial.printf("Schedule for %s: %s\n", device.c_str(), enabled ? "ENABLED" : "DISABLED");
    }

    void setScheduleOnTime(const String& device, const String& onTime) {
        int h = -1, m = -1;
        if (sscanf(onTime.c_str(), "%d:%d", &h, &m) != 2) return;
        if (device == "indoor_light") { indoorOnHour = h; indoorOnMin = m; }
        else if (device == "fan") { fanOnHour = h; fanOnMin = m; }
        Serial.printf("Schedule ON %s: %02d:%02d\n", device.c_str(), h, m);
    }

    void setScheduleOffTime(const String& device, const String& offTime) {
        int h = -1, m = -1;
        if (sscanf(offTime.c_str(), "%d:%d", &h, &m) != 2) return;
        if (device == "indoor_light") { indoorOffHour = h; indoorOffMin = m; }
        else if (device == "fan") { fanOffHour = h; fanOffMin = m; }
        Serial.printf("Schedule OFF %s: %02d:%02d\n", device.c_str(), h, m);
    }

    void setScheduleDays(const String& device, int days) {
        if (device == "indoor_light") indoorScheduleDays = days;
        else if (device == "fan") fanScheduleDays = days;
        Serial.printf("Schedule days for %s: %d\n", device.c_str(), days);
    }

    void setScheduleDay(const String& device, const String& dayName, bool enabled) {
        int bit = dayNameToBit(dayName);
        if (bit < 0) return;
        int& days = (device == "indoor_light") ? indoorScheduleDays : fanScheduleDays;
        if (enabled) days |= (1 << bit);
        else days &= ~(1 << bit);
    }


private:
    void writeRelay(uint8_t pin, bool turnOn) {
        pcf.write(pin, turnOn ? LOW : HIGH);
    }

    void checkOneSchedule(bool enabled, int onH, int onM, int offH, int offM, int days,
                          int& lastH, int& lastM, bool& status,
                          void (LightController::*setter)(bool), const char* label,
                          int curH, int curM, int dow) {
        if (!enabled || onH == -1 || offH == -1 || !(days & (1 << dow)))
            return;

        if (curH == onH && curM == onM && (lastH != curH || lastM != curM) && !status) {
            lastH = curH; lastM = curM;
            (this->*setter)(true);
            fb->logEvent("schedule", String("Hẹn giờ: Bật ") + label + ".");
        } else if (curH == offH && curM == offM && (lastH != curH || lastM != curM) && status) {
            lastH = curH; lastM = curM;
            (this->*setter)(false);
            fb->logEvent("schedule", String("Hẹn giờ: Tắt ") + label + ".");
        }
    }

    void checkSchedules() {
        time_t rawtime;
        struct tm* timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        if (timeinfo->tm_year < 120) return;

        int curH = timeinfo->tm_hour;
        int curM = timeinfo->tm_min;
        int dow = timeinfo->tm_wday;

        checkOneSchedule(indoorScheduleEnabled, indoorOnHour, indoorOnMin,
                         indoorOffHour, indoorOffMin, indoorScheduleDays,
                         lastIndoorTriggerHour, lastIndoorTriggerMin, indoorStatus,
                         &LightController::setIndoorLight, "đèn trong nhà",
                         curH, curM, dow);

        checkOneSchedule(fanScheduleEnabled, fanOnHour, fanOnMin,
                         fanOffHour, fanOffMin, fanScheduleDays,
                         lastFanTriggerHour, lastFanTriggerMin, fanStatus,
                         &LightController::setFan, "quạt",
                         curH, curM, dow);
    }
};

#endif // LIGHT_CONTROLLER_H
