#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "Secrets.h"

#define EEPROM_SIZE           96
#define AP_SSID               "SmartHome-Assistant-Config"
#define AP_IP_ADDR            192, 168, 1, 68
#define AP_SUBNET_ADDR        255, 255, 255, 0

// I2C OLED SSD1306
#define OLED_SDA              21
#define OLED_SCL              22
#define OLED_WIDTH            128
#define OLED_HEIGHT           64
#define OLED_ADDR             0x3C
#define OLED_RESET_PIN        -1

#endif
