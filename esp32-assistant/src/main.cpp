#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "Secrets.h" // Nhận cấu hình HOST và API KEY của Firebase từ bước 1
#include "WifiHelper.h"

// Cấu hình Nút bấm vật lý trên ESP32 để kích hoạt lệnh test (nếu có)
#define TEST_BUTTON_PIN 0 // Nút BOOT tích hợp trên mạch ESP32 DevKit (GPIO 0)

FirebaseData fbData;
FirebaseAuth auth;
FirebaseConfig config;
bool lastButtonState = HIGH;

// Hàm khởi tạo Firebase (Đăng nhập Anonymous tương tự ESP8266)
void initFirebase() {
  config.host = FIREBASE_HOST;
  config.database_url = "https://" FIREBASE_HOST "/";
  config.api_key = FIREBASE_API_KEY;
  
  config.token_status_callback = tokenStatusCallback;
  
  Serial.print("Signing up anonymously... ");
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
  } else {
    Serial.printf("failed: %s\n", config.signer.signupError.message.c_str());
  }
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase initialized.");
}

// Hàm cốt lõi: Gửi lệnh thay đổi trạng thái thiết bị lên Firebase
bool controlDevice(String deviceName, String key, bool status) {
  if (Firebase.ready()) {
    // Đường dẫn trên Firebase sẽ có dạng: /devices/indoor_light/status
    String path = "/devices/" + deviceName + "/" + key;
    if (Firebase.RTDB.setBool(&fbData, path.c_str(), status)) {
      Serial.printf(">>> Đã gửi lệnh điều khiển [%s/%s] -> %s thành công!\n", 
                    deviceName.c_str(), key.c_str(), status ? "BẬT" : "TẮT");
      return true;
    } else {
      Serial.print("Gửi lệnh thất bại: ");
      Serial.println(fbData.errorReason());
      return false;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP); // Thiết lập nút nhấn BOOT
  
  WifiHelper::init();
  initFirebase();
  
  Serial.println("Chờ 3 giây để hệ thống ổn định kết nối...");
  delay(3000);
  
  // Test thử: Tự động BẬT đèn trong nhà ngay khi ESP32 khởi động xong
  controlDevice("indoor_light", "status", true);
}

void loop() {
  WifiHelper::handleClient();
  WifiHelper::keepAlive();

  // Lắng nghe sự kiện nhấn nút BOOT trên ESP32 để ĐẢO TRẠNG THÁI ĐÈN thử nghiệm
  bool currentButtonState = digitalRead(TEST_BUTTON_PIN);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    delay(50); // Chống rung phím (debounce)
    static bool lightState = false;
    lightState = !lightState; // Đảo trạng thái
    
    // Gửi lệnh điều khiển bật/tắt đèn trong nhà
    controlDevice("indoor_light", "status", lightState);
  }
  lastButtonState = currentButtonState;
  
  delay(10);
}
