#ifndef FIREBASE_HELPER_H
#define FIREBASE_HELPER_H

#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "Config.h"

// Callback nhận cập nhật thiết bị: deviceName (ví dụ "indoor_light"), key ("status" hoặc "mode"), value ("true", "false", "open", "closed", etc.)
typedef void (*DeviceControlCallback)(const String& device, const String& key, const String& value);

class FirebaseHelper;
extern FirebaseHelper* fbHelperInstance;

class FirebaseHelper {
private:
    FirebaseData fbData;
    FirebaseData fbStream;
    FirebaseAuth auth;
    FirebaseConfig config;
    bool isReady = false;

    DeviceControlCallback controlCallback = nullptr;

public:
    FirebaseHelper() {
        fbHelperInstance = this;
    }

    void init(DeviceControlCallback cb) {
        controlCallback = cb;
        
        Serial.println("Initializing Firebase...");
        config.host = FIREBASE_HOST;
        config.api_key = FIREBASE_API_KEY;

        // Đăng nhập vô danh (Anonymous sign-in)
        auth.user.email = "";
        auth.user.password = "";

        config.token_status_callback = tokenStatusCallback;
        
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        
        Serial.println("Connecting to Firebase...");
        
        int timeout = 0;
        while (!Firebase.ready() && timeout < 10) {
            delay(500);
            Serial.print(".");
            timeout++;
        }
        
        if (Firebase.ready()) {
            Serial.println("\nFirebase Client Ready!");
            isReady = true;
            startStream();
        } else {
            Serial.println("\nFirebase Connection Timeout. Se tiep tuc thu trong background.");
        }
    }

    void keepAlive() {
        if (Firebase.ready()) {
            isReady = true;
        } else {
            isReady = false;
        }
    }

    bool ready() {
        return isReady && Firebase.ready();
    }

    bool setFloat(const String& path, float val) {
        if (!ready()) return false;
        return Firebase.RTDB.setFloat(&fbData, path.c_str(), val);
    }

    bool setBool(const String& path, bool val) {
        if (!ready()) return false;
        return Firebase.RTDB.setBool(&fbData, path.c_str(), val);
    }

    bool setInt(const String& path, int val) {
        if (!ready()) return false;
        return Firebase.RTDB.setInt(&fbData, path.c_str(), val);
    }

    bool setString(const String& path, const String& val) {
        if (!ready()) return false;
        return Firebase.RTDB.setString(&fbData, path.c_str(), val.c_str());
    }

    // Đọc trạng thái từ Firebase để xác thực thẻ RFID
    bool checkRFIDCard(const String& cardUID, String& name, bool& active) {
        if (!ready()) return false;
        String path = "/rfid/cards/" + cardUID;
        if (Firebase.RTDB.getJSON(&fbData, path.c_str())) {
            FirebaseJson &json = fbData.jsonObject();
            FirebaseJsonData result;
            
            json.get(result, "name");
            if (result.success) name = result.to<String>();
            else name = "Unknown";
            
            json.get(result, "active");
            if (result.success) active = result.to<bool>();
            else active = false;
            
            return true;
        }
        return false;
    }

    // Ghi log sự kiện hệ thống
    void logEvent(const String& type, const String& message) {
        if (!ready()) return;

        time_t now = time(nullptr);
        // Dùng Unix epoch ms nếu NTP đã sync (now > năm 2001), ngược lại fallback millis()
        double ts = (now > 1000000000L) ? (double)now * 1000.0 : (double)millis();

        FirebaseJson json;
        json.add("timestamp", ts);
        json.add("type", type.c_str());
        json.add("message", message.c_str());

        Firebase.RTDB.pushJSON(&fbData, "/logs/event_logs", &json);
    }

    // Ghi log quét thẻ RFID
    void logRFIDAccess(const String& cardUID, const String& name, const String& status) {
        if (!ready()) return;

        time_t now = time(nullptr);
        double ts = (now > 1000000000L) ? (double)now * 1000.0 : (double)millis();

        FirebaseJson json;
        json.add("timestamp", ts);
        json.add("card_uid", cardUID.c_str());
        json.add("name", name.c_str());
        json.add("status", status.c_str());

        Firebase.RTDB.pushJSON(&fbData, "/rfid/access_logs", &json);
    }

    void handleStreamUpdate(const String& path, FirebaseStream& data) {
        if (controlCallback == nullptr) return;

        // Path có dạng: /indoor_light/status hoặc /
        // Phân tách path để tìm tên thiết bị và thuộc tính thay đổi
        if (path == "/") {
            // Nhận toàn bộ json của /devices khi mới khởi chạy stream
            if (data.dataType() == "json") {
                FirebaseJson &json = data.jsonObject();
                // Ta có thể parse toàn bộ json ở đây nếu cần thiết lập trạng thái ban đầu
                // Để đơn giản hơn, ta sẽ cập nhật các thiết bị chính
                parseAndNotifyAll(json);
            }
            return;
        }

        // Tách chuỗi path (ví dụ: "/indoor_light/status")
        int firstSlash = path.indexOf('/', 1);
        String device = "";
        String key = "";
        
        if (firstSlash == -1) {
            device = path.substring(1);
            // Nếu set cả cục của device (ví dụ /indoor_light -> json)
            if (data.dataType() == "json") {
                FirebaseJson &json = data.jsonObject();
                parseAndNotifyDevice(device, json);
                return;
            }
            
            // Xử lý khi ghi nhận thay đổi kiểu dữ liệu cơ bản (ví dụ: /fan -> boolean)
            String valStr = "";
            if (data.dataType() == "boolean") {
                valStr = data.boolData() ? "true" : "false";
            } else if (data.dataType() == "string") {
                valStr = data.stringData();
            } else if (data.dataType() == "int") {
                valStr = String(data.intData());
            }
            
            if (device.length() > 0) {
                controlCallback(device, "", valStr);
            }
            return;
        } else {
            device = path.substring(1, firstSlash);
            key = path.substring(firstSlash + 1);
        }

        String valStr = "";
        if (data.dataType() == "boolean") {
            valStr = data.boolData() ? "true" : "false";
        } else if (data.dataType() == "string") {
            valStr = data.stringData();
        } else if (data.dataType() == "int") {
            valStr = String(data.intData());
        }

        if (device.length() > 0 && key.length() > 0) {
            controlCallback(device, key, valStr);
        }
    }

private:
    void startStream() {
        if (!Firebase.RTDB.beginStream(&fbStream, "/devices")) {
            Serial.printf("Stream begin error, %s\n", fbStream.errorReason().c_str());
            return;
        }
        
        Firebase.RTDB.setStreamCallback(&fbStream, 
            [](FirebaseStream data) {
                if (fbHelperInstance != nullptr) {
                    fbHelperInstance->handleStreamUpdate(data.dataPath(), data);
                }
            }, 
            [](bool timeout) {
                if (timeout) Serial.println("Stream timeout, resuming...");
            }
        );
    }

    void parseAndNotifyDevice(const String& device, FirebaseJson& json) {
        size_t len = json.iteratorBegin();
        String key, value;
        int type = 0;
        for (size_t i = 0; i < len; i++) {
            json.iteratorGet(i, type, key, value);
            controlCallback(device, key, value);
        }
        json.iteratorEnd();
    }

    void parseAndNotifyAll(FirebaseJson& json) {
        size_t len = json.iteratorBegin();
        String key, value;
        int type = 0;
        for (size_t i = 0; i < len; i++) {
            json.iteratorGet(i, type, key, value);
            FirebaseJson deviceJson;
            deviceJson.setJsonData(value);
            parseAndNotifyDevice(key, deviceJson);
        }
        json.iteratorEnd();
    }

};

#endif // FIREBASE_HELPER_H
