#include "FirebaseHelper.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <time.h>

FirebaseHelper::FirebaseHelper() {
    fbHelperInstance = this;
}

void FirebaseHelper::init(DeviceControlCallback cb) {
    controlCallback = cb;
    
    // Kiểm tra cấu hình mặc định (placeholder)
    if (String(FIREBASE_HOST) == "your-project-default-rtdb.asia-southeast1.firebasedatabase.app" ||
        String(FIREBASE_API_KEY) == "your_firebase_api_key" ||
        String(FIREBASE_HOST).length() == 0 ||
        String(FIREBASE_API_KEY).length() == 0) {
        Serial.println("[Firebase] CANH BAO: Firebase chua duoc cau hinh hop le. Chay o che do OFFLINE cuc bo.");
        initialized = false;
        isReady = false;
        return;
    }
    
    initialized = true;
    Serial.println("[Firebase] Bat dau khoi tao Firebase...");
    Serial.print("[Firebase Debug] Host: ");
    Serial.println(FIREBASE_HOST);
    Serial.print("[Firebase Debug] API Key: ");
    Serial.println(FIREBASE_API_KEY);
    
    config.host = FIREBASE_HOST;
    config.database_url = "https://" FIREBASE_HOST "/";
    config.api_key = FIREBASE_API_KEY;

    // Đăng nhập ẩn danh (Anonymous) theo thiết lập bảo mật
    auth.user.email = "";
    auth.user.password = "";

    config.token_status_callback = tokenStatusCallback;
    
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    Serial.println("[Firebase] Dang ket noi...");
    
    int timeout = 0;
    while (!Firebase.ready() && timeout < 10) {
        delay(500);
        Serial.print(".");
        timeout++;
    }
    
    if (Firebase.ready()) {
        Serial.println("\n[Firebase] Client san sang hoat dong!");
        isReady = true;
        startStream();
    } else {
        Serial.println("\n[Firebase] Timeout ket noi. Se thu lai trong background.");
    }
}

void FirebaseHelper::keepAlive() {
    if (!initialized) return;
    
    bool currentlyReady = Firebase.ready();
    if (currentlyReady && !isReady) {
        Serial.println("[Firebase] Client san sang hoat dong trong background!");
        isReady = true;
        startStream();
    } else {
        isReady = currentlyReady;
    }
}

bool FirebaseHelper::ready() {
    return initialized && isReady && Firebase.ready();
}

bool FirebaseHelper::setBool(const String& path, bool val) {
    if (!ready()) return false;
    return Firebase.RTDB.setBool(&fbData, path.c_str(), val);
}

bool FirebaseHelper::setInt(const String& path, int val) {
    if (!ready()) return false;
    return Firebase.RTDB.setInt(&fbData, path.c_str(), val);
}

bool FirebaseHelper::setString(const String& path, const String& val) {
    if (!ready()) return false;
    return Firebase.RTDB.setString(&fbData, path.c_str(), val.c_str());
}

void FirebaseHelper::updateHeartbeat(const String& ip) {
    if (!ready()) return;
    
    static unsigned long lastHeartbeatTime = 0;
    if (lastHeartbeatTime == 0 || millis() - lastHeartbeatTime >= 15000) {
        lastHeartbeatTime = millis();
        
        time_t now = time(nullptr);
        double ts = (now > 1000000000L) ? (double)now * 1000.0 : (double)millis();
        
        FirebaseJson json;
        json.add("ip", ip.c_str());
        json.add("last_seen", ts);
        
        Firebase.RTDB.setJSON(&fbData, "/status/esp8266", &json);
    }
}

bool FirebaseHelper::checkRFIDCard(const String& cardUID, String& name, bool& active) {
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

void FirebaseHelper::logEvent(const String& type, const String& message) {
    if (!ready()) return;

    time_t now = time(nullptr);
    double ts = (now > 1000000000L) ? (double)now * 1000.0 : (double)millis();

    FirebaseJson json;
    json.add("timestamp", ts);
    json.add("type", type.c_str());
    json.add("message", message.c_str());

    Firebase.RTDB.pushJSON(&fbData, "/logs/event_logs", &json);
    Firebase.RTDB.setJSON(&fbData, "/status/last_event", &json);
}

void FirebaseHelper::logRFIDAccess(const String& cardUID, const String& name, const String& status) {
    if (!ready()) return;

    time_t now = time(nullptr);
    double ts = (now > 1000000000L) ? (double)now * 1000.0 : (double)millis();

    FirebaseJson json;
    json.add("timestamp", ts);
    json.add("card_uid", cardUID.c_str());
    json.add("name", name.c_str());
    json.add("status", status.c_str());

    Firebase.RTDB.pushJSON(&fbData, "/rfid/access_logs", &json);
    Firebase.RTDB.setJSON(&fbData, "/status/last_rfid", &json);
}

void FirebaseHelper::updateSensors(float temp, float hum) {
    if (!ready()) return;

    static unsigned long lastSensorPush = 0;
    if (millis() - lastSensorPush < 10000) return;
    lastSensorPush = millis();

    time_t now = time(nullptr);
    double ts = (now > 1000000000L) ? (double)now * 1000.0 : (double)millis();

    Firebase.RTDB.setFloat(&fbData, "/sensors/temperature", temp);
    Firebase.RTDB.setFloat(&fbData, "/sensors/humidity", hum);
    Firebase.RTDB.setDouble(&fbData, "/sensors/last_updated", ts);

    static unsigned long lastHistoryPush = 0;
    if (lastHistoryPush == 0 || millis() - lastHistoryPush >= 300000) {
        lastHistoryPush = millis();
        FirebaseJson hJson;
        hJson.add("timestamp", ts);
        hJson.add("temperature", temp);
        hJson.add("humidity", hum);
        Firebase.RTDB.pushJSON(&fbData, "/sensors/history", &hJson);
    }
}

void FirebaseHelper::handleStreamUpdate(const String& path, FirebaseStream& data) {
    if (controlCallback == nullptr) return;

    if (path == "/") {
        if (data.dataType() == "json") {
            FirebaseJson &json = data.jsonObject();
            parseAndNotifyAll(json);
        }
        return;
    }

    int firstSlash = path.indexOf('/', 1);
    String device = "";
    String key = "";
    
    if (firstSlash == -1) {
        device = path.substring(1);
        if (data.dataType() == "json") {
            FirebaseJson &json = data.jsonObject();
            parseAndNotifyDevice(device, json);
            return;
        }
        
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

void FirebaseHelper::startStream() {
    if (!Firebase.RTDB.beginStream(&fbStream, "/devices")) {
        Serial.printf("[Firebase] Loi khoi chay stream: %s\n", fbStream.errorReason().c_str());
        return;
    }
    
    Firebase.RTDB.setStreamCallback(&fbStream, 
        [](FirebaseStream data) {
            if (fbHelperInstance != nullptr) {
                fbHelperInstance->handleStreamUpdate(data.dataPath(), data);
            }
        }, 
        [](bool timeout) {
            if (timeout) Serial.println("[Firebase] Stream timeout, dang khoi phuc...");
        }
    );
}

void FirebaseHelper::parseAndNotifyDevice(const String& device, FirebaseJson& json) {
    size_t len = json.iteratorBegin();
    String key, value;
    int type = 0;
    for (size_t i = 0; i < len; i++) {
        json.iteratorGet(i, type, key, value);
        controlCallback(device, key, value);
    }
    json.iteratorEnd();
}

void FirebaseHelper::parseAndNotifyAll(FirebaseJson& json) {
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
