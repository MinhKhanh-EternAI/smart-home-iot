#ifndef ROOF_CONTROLLER_H
#define ROOF_CONTROLLER_H

#include <Servo.h>
#include "Config.h"
#include "FirebaseHelper.h"
#include "WifiHelper.h"

class RoofController {
private:
    Servo roofServo;
    FirebaseHelper* fb;
    
    bool isClosed = false;
    String mode = "auto"; // "auto" hoặc "manual"
    unsigned long lastSensorRead = 0;
    const unsigned long sensorReadInterval = 3000; // Đọc cảm biến nước mỗi 3 giây
    unsigned long lastToggleTime = 0;
    const unsigned long toggleCooldown = 30000; // Chờ 30s mới cho phép đổi trạng thái

public:
    RoofController() {}

    void init(FirebaseHelper* fbHelper) {
        fb = fbHelper;
        
        roofServo.attach(SERVO_ROOF_PIN);
        roofServo.write(ROOF_OPEN_ANGLE);
        Serial.printf("Servo Roof attached to Pin: %d\n", SERVO_ROOF_PIN);
        
        isClosed = false;
    }

    void update() {
        unsigned long now = millis();
        if (now - lastSensorRead >= sensorReadInterval) {
            lastSensorRead = now;
            
            // Đọc giá trị Analog của Water Sensor (cảm biến mưa)
            int sensorVal = analogRead(WATER_SENSOR_PIN);
            bool isRaining = (sensorVal < RAIN_THRESHOLD); // Điện trở giảm khi ướt -> điện áp thấp hơn
            
            // Cập nhật trạng thái mưa lên Firebase
            fb->setBool("/sensors/rain", isRaining);
            
            // Chế độ Tự động (Auto Mode) — có cooldown chống flapping
            if (mode == "auto" && (now - lastToggleTime >= toggleCooldown)) {
                if (isRaining && !isClosed) {
                    closeRoof();
                    lastToggleTime = millis();
                    fb->logEvent("automation", "Tự động đóng mái che do phát hiện trời mưa.");
                } else if (!isRaining && isClosed) {
                    openRoof();
                    lastToggleTime = millis();
                    fb->logEvent("automation", "Tự động mở mái che do trời đã tạnh mưa.");
                }
            }
        }
    }

    void openRoof() {
        roofServo.write(ROOF_OPEN_ANGLE);
        isClosed = false;
        fb->setString("/devices/roof/status", "open");
        Serial.println("Roof opened.");
        WifiHelper::showNotification("Mai che", "DA MO");
    }

    void closeRoof() {
        roofServo.write(ROOF_CLOSE_ANGLE);
        isClosed = true;
        fb->setString("/devices/roof/status", "closed");
        Serial.println("Roof closed.");
        WifiHelper::showNotification("Mai che", "DA DONG");
    }

    void setStatusFromFirebase(const String& status) {
        if (mode != "manual") return;
        if (status == "open") {
            openRoof();
        } else if (status == "closed") {
            closeRoof();
        }
    }

    void setMode(const String& newMode) {
        if (newMode == "auto" || newMode == "manual") {
            mode = newMode;
            Serial.printf("Roof mode updated to: %s\n", mode.c_str());
            fb->setString("/devices/roof/mode", mode);
        }
    }
};

#endif // ROOF_CONTROLLER_H
