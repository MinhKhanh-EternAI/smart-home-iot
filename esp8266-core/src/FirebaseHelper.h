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
    static const unsigned long HEARTBEAT_INTERVAL = 15000;
    static const unsigned long STREAM_RETRY_INTERVAL = 30000;

    FirebaseData fbData;
    FirebaseData fbStream;
    FirebaseAuth auth;
    FirebaseConfig config;
    bool isReady = false;
    bool streamStarted = false;
    unsigned long lastStreamRetry = 0;

    DeviceControlCallback controlCallback = nullptr;

    static double currentTimestamp() {
        time_t now = time(nullptr);
        return (now > 1000000000L) ? (double)now * 1000.0 : (double)millis();
    }

public:
    FirebaseHelper() {
        fbHelperInstance = this;
    }

    void init(DeviceControlCallback cb) {
        controlCallback = cb;
        
        Serial.println("Initializing Firebase...");
        config.host = FIREBASE_HOST;
        config.database_url = "https://" FIREBASE_HOST "/";
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
            if (!streamStarted) {
                startStream();
            }
        } else {
            isReady = false;
            if (streamStarted && millis() - lastStreamRetry >= STREAM_RETRY_INTERVAL) {
                lastStreamRetry = millis();
                startStream();
            }
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

    void updateHeartbeat(const String& ip) {
        if (!ready()) return;
        
        static unsigned long lastHeartbeatTime = 0;
        if (lastHeartbeatTime == 0 || millis() - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
            lastHeartbeatTime = millis();
            
            FirebaseJson json;
            json.add("ip", ip.c_str());
            json.add("last_seen", currentTimestamp());
            
            Firebase.RTDB.setJSON(&fbData, "/status/esp8266", &json);
        }
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

        FirebaseJson json;
        json.add("timestamp", currentTimestamp());
        json.add("type", type.c_str());
        json.add("message", message.c_str());

        Firebase.RTDB.pushJSON(&fbData, "/logs/event_logs", &json);
    }

    // Ghi log quét thẻ RFID
    void logRFIDAccess(const String& cardUID, const String& name, const String& status) {
        if (!ready()) return;

        FirebaseJson json;
        json.add("timestamp", currentTimestamp());
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
            streamStarted = false;
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
        streamStarted = true;
        lastStreamRetry = millis();
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
            if (value.length() > 0 && value[0] == '{') {
                FirebaseJson deviceJson;
                deviceJson.setJsonData(value);
                parseAndNotifyDevice(key, deviceJson);
            } else {
                controlCallback(key, "", value);
            }
        }
        json.iteratorEnd();
    }

};

#endif // FIREBASE_HELPER_H
