#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#if USE_PCF8574
#include <PCF8574.h>
#endif
#include "Config.h"
#include "FirebaseHelper.h"

class LightController {
private:
#if USE_PCF8574
    PCF8574 pcf;
#endif
    FirebaseHelper* fb;
    bool lightStatus = false;

public:
#if USE_PCF8574
    LightController() : pcf(PCF_I2C_ADDRESS), fb(nullptr) {}
#else
    LightController() : fb(nullptr) {}
#endif

    void init(FirebaseHelper* fbHelper) {
        fb = fbHelper;
        
#if USE_PCF8574
        // Khởi động bus PCF8574
        pcf.begin();
        // Trạng thái ban đầu: Tắt đèn (Ghi HIGH vì relay Active Low)
        pcf.write(PCF_RELAY_LIGHT, HIGH);
        Serial.println("[LightController] PCF8574 Relay khoi tao thanh cong (Pin P1).");
#else
        // Khởi động trực tiếp GPIO
        pinMode(DIRECT_RELAY_PIN, OUTPUT);
        digitalWrite(DIRECT_RELAY_PIN, LOW); // Mặc định tắt (Active High)
        Serial.printf("[LightController] Direct GPIO Relay khoi tao thanh cong (Pin: %d).\n", DIRECT_RELAY_PIN);
#endif
    }

    void update() {
        // Rút gọn, không sử dụng Hẹn giờ (Schedule) và tự động bật theo chuyển động PIR
    }

    void setIndoorLight(bool status) {
        lightStatus = status;
        
#if USE_PCF8574
        // Active Low: status = true (bật) -> ghi LOW, status = false (tắt) -> ghi HIGH
        pcf.write(PCF_RELAY_LIGHT, status ? LOW : HIGH);
#else
        digitalWrite(DIRECT_RELAY_PIN, status ? HIGH : LOW); // Active High
#endif

        Serial.printf("[LightController] Den 12V thiet lap trang thai: %s\n", status ? "ON" : "OFF");
        
        // Đồng bộ lên Firebase (sử dụng đường dẫn cũ "/devices/indoor_light/status" để tương thích với Dashboard)
        if (fb != nullptr && fb->ready()) {
            fb->setBool("/devices/indoor_light/status", status);
        }
    }

    bool getLightStatus() const {
        return lightStatus;
    }
};

#endif // LIGHT_CONTROLLER_H
