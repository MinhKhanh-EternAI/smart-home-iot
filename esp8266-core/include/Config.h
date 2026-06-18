#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. Cấu hình WiFi & Firebase
// ==========================================
#define DEFAULT_WIFI_SSID     "Your_WiFi_SSID"
#define DEFAULT_WIFI_PASSWORD "Your_WiFi_Password"
#define EEPROM_SIZE           96
#define AP_SSID               "SmartHome-Config"

#define FIREBASE_HOST         "esp32-smart-home-c7e8a-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_API_KEY      "AIzaSyCnupBCvmT1P7ItpOTOCe0HGA1vovytwbQ"

// ==========================================
// 2. Tùy chọn Tối ưu hóa Chân điều khiển
// ==========================================
// CHỌN CẤU HÌNH CHÂN:
// - Đặt 1: Sử dụng cấu hình chân Tối ưu (Tránh rung Servo ở RX/TX, PIR đưa lên PCF8574).
// - Đặt 0: Sử dụng cấu hình chân Gốc theo yêu cầu ban đầu (Servo ở RX/TX, PIR ở D4).
#define USE_OPTIMIZED_PINS 1

// ==========================================
// 3. Định nghĩa chân GPIO của ESP8266
// ==========================================
// Giao tiếp SPI cho RFID RC522
#define RC522_SS_PIN          D8   // GPIO15
#define RC522_RST_PIN         D0   // GPIO16
#define RC522_SCK_PIN         D5   // GPIO14
#define RC522_MOSI_PIN        D7   // GPIO13
#define RC522_MISO_PIN        D6   // GPIO12

// Giao tiếp I2C cho LCD và PCF8574
#define I2C_SDA_PIN           D2   // GPIO4
#define I2C_SCL_PIN           D1   // GPIO5

// Cảm biến DHT11 và Cảm biến Nước
#define DHT_PIN               D3   // GPIO0
#define WATER_SENSOR_PIN      A0   // Analog input

#if USE_OPTIMIZED_PINS
  // Cấu hình chân Tối ưu (Khuyên dùng)
  #define SERVO_DOOR_PIN      D4   // GPIO2 (An toàn cho Servo)
  #define SERVO_ROOF_PIN      D4   // Có thể thay đổi thành chân khác như RX/TX nếu cần 2 servo, nhưng an toàn hơn.
                                   // Hoặc sử dụng RX (GPIO3) cho Servo Roof nhưng tắt Serial để giảm nhiễu.
  #define SERVO_ROOF_ALT_PIN  D3   // Hoặc chia sẻ hoặc dùng RX/TX
  #define PIR_PIN_ESP         -1   // Không dùng chân ESP, PIR kết nối vào PCF8574 P3
#else
  // Cấu hình chân Gốc theo yêu cầu
  #define SERVO_DOOR_PIN      3    // RX (GPIO3)
  #define SERVO_ROOF_PIN      1    // TX (GPIO1)
  #define PIR_PIN_ESP         D4   // GPIO2
#endif

// ==========================================
// 4. Định nghĩa chân mở rộng trên PCF8574
// ==========================================
#define PCF_I2C_ADDRESS       0x20 // Địa chỉ I2C mặc định của PCF8574 (có thể là 0x20, 0x27, 0x3F)

#define PCF_RELAY_OUTDOOR     0    // P0
#define PCF_RELAY_INDOOR      1    // P1
#define PCF_BUZZER            2    // P2
#define PCF_FAN_RELAY         4    // P4

#if USE_OPTIMIZED_PINS
  #define PCF_PIR_INPUT       3    // P3 (Cảm biến PIR chuyển sang chân này)
#endif

// ==========================================
// 5. Cấu hình ngưỡng cảm biến & thời gian
// ==========================================
#define RAIN_THRESHOLD        500  // Ngưỡng phát hiện mưa trên Water Sensor A0 (0-1023, giá trị nhỏ hơn = nhiều nước hơn)
#define DOOR_OPEN_ANGLE       90   // Góc mở Servo Cửa
#define DOOR_CLOSE_ANGLE      0    // Góc đóng Servo Cửa
#define ROOF_OPEN_ANGLE       90   // Góc mở Servo Mái
#define ROOF_CLOSE_ANGLE      0    // Góc đóng Servo Mái

#endif // CONFIG_H
