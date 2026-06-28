#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "Secrets.h"

// Cấu hình Wi-Fi cục bộ phát ra từ ESP8266 (AP Mode)
#define AP_SSID               "SmartHome-AP"
#define AP_PASS               "12345678"          // Mật khẩu Wi-Fi AP
#define AP_IP_ADDR            192, 168, 4, 1      // IP mặc định của AP tránh trùng với lớp mạng Router (thường là 192.168.1.x)
#define AP_SUBNET_ADDR        255, 255, 255, 0

// Kích thước EEPROM để lưu SSID và mật khẩu Wi-Fi cấu hình
#define EEPROM_SIZE           96

// --- CẤU HÌNH PHẦN CỨNG ---

// SPI cho RFID RC522
#define RC522_SS_PIN          D8
#define RC522_RST_PIN         D4   // Chân ảo (đã nối cứng RST của RFID vào 3.3V)
#define RC522_SCK_PIN         D5
#define RC522_MOSI_PIN        D7
#define RC522_MISO_PIN        D6

// I2C cho màn hình LCD và Mạch mở rộng PCF8574
#define I2C_SDA_PIN           D2
#define I2C_SCL_PIN           D1

// Chân điều khiển Servo cửa MG90S (PWM trực tiếp từ ESP8266)
#define SERVO_DOOR_PIN        D4

// Lựa chọn cách đấu nối Relay và Buzzer:
// 1: Dùng mạch mở rộng PCF8574 qua I2C (theo cấu hình ban đầu)
// 0: Đấu nối trực tiếp vào các chân GPIO của ESP8266
#define USE_PCF8574           0

// Cấu hình khi dùng PCF8574 (I2C Address mặc định thường là 0x20)
#define PCF_I2C_ADDRESS       0x20
#define PCF_RELAY_LIGHT       1    // Chân P1 trên PCF8574 điều khiển Relay Đèn 12V (Active Low)
#define PCF_BUZZER            2    // Chân P2 trên PCF8574 điều khiển Buzzer (Active Low)

// Cấu hình khi đấu trực tiếp GPIO (Chỉ có hiệu lực khi USE_PCF8574 = 0)
#define DIRECT_RELAY_PIN      D0   // Chân GPIO điều khiển Relay
#define DIRECT_BUZZER_PIN     D3   // Chân GPIO điều khiển Buzzer
#define DIRECT_DHT_PIN        3    // Chân RX (GPIO3) điều khiển cảm biến DHT11
#define DIRECT_DHT_TYPE       11   // Loại cảm biến (11 tương ứng với DHT11 trong thư viện Adafruit)

// Góc quay của Servo MG90S cho cửa
#define DOOR_OPEN_ANGLE       0
#define DOOR_CLOSE_ANGLE      180

#endif // CONFIG_H
