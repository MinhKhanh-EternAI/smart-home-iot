#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Secrets.h"
#include "Config.h"
#include "WifiHelper.h"

#define TEST_BUTTON_PIN 0

FirebaseData fbData;
FirebaseAuth auth;
FirebaseConfig config;
bool lastButtonState = HIGH;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN);

void initOLED() {
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed!");
    return;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32 SmartHome");
  display.println("Starting...");
  display.display();
  Serial.println("OLED initialized.");
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);

  // Dòng 1: WiFi status
  display.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    display.print("OK ");
    display.print(WiFi.localIP());
  } else {
    display.print("Connecting...");
  }

  // Dòng 2: Firebase status
  display.setCursor(0, 16);
  display.print("Firebase: ");
  display.print(Firebase.ready() ? "OK" : "Wait");

  // Dòng 3: Thời gian
  display.setCursor(0, 32);
  time_t now = time(nullptr);
  if (now > 1000000000L) {
    struct tm *ti = localtime(&now);
    char buf[20];
    sprintf(buf, "%02d:%02d:%02d", ti->tm_hour, ti->tm_min, ti->tm_sec);
    display.print("Time: ");
    display.print(buf);
  }

  // Dòng 4: Sẵn sàng
  display.setCursor(0, 48);
  display.print("System ready");

  display.display();
}

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
  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP);
  
  initOLED();
  
  // Hiển thị thông báo kết nối WiFi
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();
  
  WifiHelper::init();
  initFirebase();
  
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("NTP Time Sync initialized on ESP32.");
  
  Serial.println("Waiting 3s for stable connection...");
  delay(3000);
  
  updateDisplay();
  controlDevice("indoor_light", "status", true);
}

void updateHeartbeat() {
  static unsigned long lastHeartbeatTime = 0;
  if (lastHeartbeatTime == 0 || millis() - lastHeartbeatTime >= 15000) {
    if (Firebase.ready() && WiFi.status() == WL_CONNECTED) {
      lastHeartbeatTime = millis();
      
      time_t now = time(nullptr);
      double ts = (now > 1000000000L) ? (double)now * 1000.0 : (double)millis();
      
      FirebaseJson json;
      json.add("ip", WiFi.localIP().toString().c_str());
      json.add("last_seen", ts);
      
      if (Firebase.RTDB.setJSON(&fbData, "/status/esp32", &json)) {
        // Heartbeat sent successfully
      }
    }
  }
}

void loop() {
  static unsigned long lastDisplayUpdate = 0;
  const unsigned long displayInterval = 2000;

  WifiHelper::handleClient();
  WifiHelper::keepAlive();
  
  updateHeartbeat();

  // Cập nhật màn hình mỗi 2 giây
  if (millis() - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }

  bool currentButtonState = digitalRead(TEST_BUTTON_PIN);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    delay(50);
    static bool lightState = false;
    lightState = !lightState;
    
    controlDevice("indoor_light", "status", lightState);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.print(lightState ? "LIGHT ON" : "LIGHT OFF");
    display.display();
    delay(1500);
  }
  lastButtonState = currentButtonState;
  
  delay(10);
}
