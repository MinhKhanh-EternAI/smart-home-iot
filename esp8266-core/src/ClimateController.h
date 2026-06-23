#ifndef CLIMATE_CONTROLLER_H
#define CLIMATE_CONTROLLER_H

#include <DHT.h>
#include <time.h>
#include "Config.h"
#include "FirebaseHelper.h"

class ClimateController {
private:
    DHT dht;
    FirebaseHelper* fb;
    
    float lastTemp = -999.0;
    float lastHum = -999.0;
    unsigned long lastReadTime = 0;
    const unsigned long readInterval = 10000; // Đọc nhiệt độ/độ ẩm mỗi 10 giây

public:
    ClimateController() : dht(DHT_PIN, DHT11) {}

    void init(FirebaseHelper* fbHelper) {
        fb = fbHelper;
        dht.begin();
        Serial.println("DHT11 sensor initialized.");
    }

    void update() {
        unsigned long now = millis();
        if (now - lastReadTime >= readInterval) {
            lastReadTime = now;
            
            float h = dht.readHumidity();
            float t = dht.readTemperature();
            
            // Kiểm tra xem dữ liệu đọc có hợp lệ không
            if (isnan(h) || isnan(t)) {
                Serial.println("Failed to read from DHT sensor!");
                return;
            }
            
            // Chỉ cập nhật lên Firebase nếu giá trị thay đổi đáng kể (tránh spam ghi DB)
            // hoặc gửi định kỳ
            bool tempChanged = (abs(t - lastTemp) >= 0.5);
            bool humChanged = (abs(h - lastHum) >= 1.0);
            
            if (tempChanged || humChanged || lastTemp == -999.0) {
                lastTemp = t;
                lastHum = h;
                
                Serial.printf("DHT11: Temp: %.1f C, Hum: %.1f %%\n", t, h);
                
                fb->setFloat("/sensors/temperature", t);
                fb->setFloat("/sensors/humidity", h);
                
                fb->setInt("/sensors/last_updated", (int)time(nullptr));
                
                // Cảnh báo nhiệt độ quá cao (chống cháy)
                if (t >= 50.0) {
                    fb->logEvent("security", "CẢNH BÁO: Nhiệt độ vượt ngưỡng an toàn (" + String(t) + "C)!");
                }
            }
        }
    }
};

#endif // CLIMATE_CONTROLLER_H
