#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include "Config.h"

// Khai báo trước các class điều khiển để tránh lỗi include vòng
class LightController;
class DoorController;
class FirebaseHelper;

class WifiHelper {
private:
    static ESP8266WebServer& getServer() {
        static ESP8266WebServer server(80);
        return server;
    }

    static void writeStringToEEPROM(int offset, const String& str) {
        int len = str.length();
        for (int i = 0; i < len; i++) {
            EEPROM.write(offset + i, str[i]);
        }
        EEPROM.write(offset + len, '\0');
        EEPROM.commit();
    }

    static String readStringFromEEPROM(int offset, int maxLen) {
        String str = "";
        for (int i = 0; i < maxLen; i++) {
            char c = EEPROM.read(offset + i);
            if (c == '\0' || c == 0xFF) break;
            str += c;
        }
        return str;
    }

public:
    static int& getLCDAddress() {
        static int lcdAddress = -1;
        return lcdAddress;
    }

    // Các con trỏ tĩnh trỏ đến các đối tượng điều khiển trong main
    static LightController* lightCtrl;
    static DoorController* doorCtrl;
    static FirebaseHelper* fbHelper;

    static void printToLCD(const String& line1, const String& line2 = "");
    static void init();
    static void startAP();
    static void handleClient();
    static void keepAlive();
};

#endif // WIFI_HELPER_H
