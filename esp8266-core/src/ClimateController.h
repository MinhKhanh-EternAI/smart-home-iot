#ifndef CLIMATE_CONTROLLER_H
#define CLIMATE_CONTROLLER_H

#include <time.h>
#include "Config.h"
#include "FirebaseHelper.h"

#if USE_DHT
#include <DHT.h>
#endif

class ClimateController {
private:
    FirebaseHelper* fb;
    
#if USE_DHT
    DHT dht;
    float lastTemp = -999.0;
    float lastHum = -999.0;
    unsigned long lastReadTime = 0;
    const unsigned long readInterval = 10000;
#endif

public:
#if USE_DHT
    ClimateController() : dht(DHT_PIN, DHT11) {}
#else
    ClimateController() {}
#endif

    void init(FirebaseHelper* fbHelper) {
        fb = fbHelper;
#if USE_DHT
        dht.begin();
        Serial.println("DHT11 sensor initialized.");
#else
        Serial.println("ClimateController: DHT not connected (USE_DHT=0).");
#endif
    }

    void update() {
#if USE_DHT
        unsigned long now = millis();
        if (now - lastReadTime >= readInterval) {
            lastReadTime = now;
            float h = dht.readHumidity();
            float t = dht.readTemperature();
            
            if (isnan(h) || isnan(t)) {
                Serial.println("Failed to read from DHT sensor!");
                return;
            }
            
            bool tempChanged = (fabsf(t - lastTemp) >= 0.5f);
            bool humChanged = (fabsf(h - lastHum) >= 1.0f);
            
            if (tempChanged || humChanged || lastTemp == -999.0) {
                lastTemp = t;
                lastHum = h;
                
                Serial.printf("DHT11: Temp: %.1f C, Hum: %.1f %%\n", t, h);
                
                fb->setFloat("/sensors/temperature", t);
                fb->setFloat("/sensors/humidity", h);
                
                fb->setInt("/sensors/last_updated", (int)time(nullptr));
                
                if (t >= 50.0) {
                    fb->logEvent("security", "CẢNH BÁO: Nhiệt độ vượt ngưỡng an toàn (" + String(t) + "C)!");
                }
            }
        }
#endif
    }
};

#endif // CLIMATE_CONTROLLER_H
