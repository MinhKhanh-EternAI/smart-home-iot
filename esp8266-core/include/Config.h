#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "Secrets.h"

// Tăng số này mỗi lần flash code mới để tự động xoá EEPROM (WiFi, cấu hình cũ)
#define FIRMWARE_VERSION      3

#define EEPROM_SIZE           100     // 96 (SSID+pass) + 4 (version marker)
#define EEPROM_VERSION_OFFSET 96
#define AP_SSID               "SmartHome-Config"
#define AP_PASSWORD           ""        // để trống nếu muốn open, set mật khẩu nếu cần bảo mật
#define AP_IP_ADDR            192, 168, 1, 86
#define AP_SUBNET_ADDR        255, 255, 255, 0


// SPI cho RFID RC522 (SCK/MOSI/MISO mặc định theo ESP8266 SPI: D5/D7/D6)
#define RC522_SS_PIN          D8
#define RC522_RST_PIN         D0

// I2C cho LCD, PCF8574
#define I2C_SDA_PIN           D2
#define I2C_SCL_PIN           D1

// DHT11 (set 0 để tắt nếu không gắn cảm biến)
#define USE_DHT               0
#if USE_DHT
#define DHT_PIN               D3
#endif

// Cảm biến mưa
#define WATER_SENSOR_PIN      A0

// Servo
#define SERVO_DOOR_PIN        D4
#define SERVO_ROOF_PIN        3     // RX (GPIO3) — Serial.end() trước khi dùng

// PCF8574 I2C Expander
#define PCF_I2C_ADDRESS       0x20
#define PCF_RELAY_OUTDOOR     0
#define PCF_RELAY_INDOOR      1
#define PCF_BUZZER            2
#define PCF_LM393_INPUT       3
#define PCF_FAN_RELAY         4     // DC motor quạt (bật/tắt qua relay)

// Ngưỡng cảm biến
#define RAIN_THRESHOLD        500
#define DOOR_OPEN_ANGLE       90
#define DOOR_CLOSE_ANGLE      0
#define ROOF_OPEN_ANGLE       90
#define ROOF_CLOSE_ANGLE      0

#endif
