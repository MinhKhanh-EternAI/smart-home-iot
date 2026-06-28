#ifndef FIREBASE_HELPER_H
#define FIREBASE_HELPER_H

#include <Firebase_ESP_Client.h>
#include "Config.h"

// Callback nhận cập nhật thiết bị từ Firebase Stream:
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
    bool initialized = false;

    DeviceControlCallback controlCallback = nullptr;

    void startStream();
    void parseAndNotifyDevice(const String& device, FirebaseJson& json);
    void parseAndNotifyAll(FirebaseJson& json);

public:
    FirebaseHelper();

    void init(DeviceControlCallback cb);
    void keepAlive();
    bool ready();
    bool isInitialized() const { return initialized; }

    bool setBool(const String& path, bool val);
    bool setInt(const String& path, int val);
    bool setString(const String& path, const String& val);
    void updateHeartbeat(const String& ip);

    // Đọc cơ sở dữ liệu để xác thực thẻ RFID
    bool checkRFIDCard(const String& cardUID, String& name, bool& active);

    // Ghi log sự kiện hệ thống lên Firebase
    void logEvent(const String& type, const String& message);

    // Ghi nhật ký quẹt thẻ RFID lên Firebase
    void logRFIDAccess(const String& cardUID, const String& name, const String& status);

    // Đẩy dữ liệu cảm biến (nhiệt độ, độ ẩm) lên Firebase
    void updateSensors(float temp, float hum);

    void handleStreamUpdate(const String& path, FirebaseStream& data);
};

#endif // FIREBASE_HELPER_H
