#ifndef DOOR_CONTROLLER_H
#define DOOR_CONTROLLER_H

#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include "Config.h"
#include "FirebaseHelper.h"
#include "WifiHelper.h"

class DoorController {
private:
    MFRC522 mfrc522;
    LiquidCrystal_I2C lcd;
    Servo doorServo;
    FirebaseHelper* fb;
    
    bool isOpen = false;
    unsigned long openTime = 0;
    unsigned long autoCloseDelay = 5000;

    bool isDenying = false;
    unsigned long denyStartTime = 0;
    static const unsigned long DENY_DISPLAY_MS = 2500;

public:
    DoorController() : 
        mfrc522(RC522_SS_PIN, RC522_RST_PIN),
        lcd(0x27, 16, 2) // Địa chỉ LCD mặc định 0x27, màn hình 16x2
    {}

    void init(FirebaseHelper* fbHelper) {
        fb = fbHelper;
        
        // Khởi tạo SPI và RFID
        SPI.begin();
        mfrc522.PCD_Init();
        Serial.println("RFID RC522 Initialized.");

        // Khởi tạo LCD
        if (!WifiHelper::getAPMode()) {
            lcd.init();
            lcd.backlight();
            showStandbyMessage();
            Serial.println("LCD I2C Initialized.");
        } else {
            Serial.println("LCD I2C Skip Init (Preserved AP Mode Display).");
        }

        // Khởi tạo Servo
        // Nếu dùng cấu hình chân gốc RX/TX, cần lưu ý:
        // Servo.attach() sẽ bắt đầu gửi tín hiệu điều khiển ra chân đó.
        #if USE_OPTIMIZED_PINS
            doorServo.attach(SERVO_DOOR_PIN);
            doorServo.write(DOOR_CLOSE_ANGLE);
            Serial.printf("Servo Door attached to Pin: %d\n", SERVO_DOOR_PIN);
        #else
            // Với chân RX (GPIO3), ta vẫn attach bình thường nhưng tắt debug
            doorServo.attach(SERVO_DOOR_PIN);
            doorServo.write(DOOR_CLOSE_ANGLE);
            Serial.println("Servo Door attached to RX pin (GPIO3).");
        #endif
    }

    void update() {
        // Bộ đếm hiển thị từ chối (non-blocking — thay thế delay(2500) cũ)
        if (isDenying) {
            if (millis() - denyStartTime >= DENY_DISPLAY_MS) {
                isDenying = false;
                showStandbyMessage();
            }
            return;
        }

        // Tự động đóng cửa sau thời gian cấu hình
        if (isOpen && (millis() - openTime >= autoCloseDelay)) {
            closeDoor();
        }

        if (!mfrc522.PICC_IsNewCardPresent()) {
            return;
        }

        if (!mfrc522.PICC_ReadCardSerial()) {
            return;
        }

        // Chuyển UID thẻ thành dạng String viết hoa
        String cardUID = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            cardUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
            cardUID += String(mfrc522.uid.uidByte[i], HEX);
        }
        cardUID.toUpperCase();
        
        Serial.print("RFID Scanned! UID: ");
        Serial.println(cardUID);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Checking card...");
        
        String holderName = "Unknown";
        bool isActive = false;

        // Xác thực với Firebase
        bool cardExists = fb->checkRFIDCard(cardUID, holderName, isActive);

        if (cardExists && isActive) {
            // Xác thực thành công
            Serial.printf("Access GRANTED to: %s\n", holderName.c_str());
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Access GRANTED");
            lcd.setCursor(0, 1);
            lcd.print(holderName.substring(0, 16)); // Hiển thị tối đa 16 ký tự tên

            fb->logRFIDAccess(cardUID, holderName, "granted");
            fb->logEvent("door", "Cửa mở bằng thẻ RFID: " + holderName);
            
            openDoor();
        } else {
            // Xác thực thất bại
            Serial.println("Access DENIED!");
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Access DENIED!");
            lcd.setCursor(0, 1);
            if (!cardExists) {
                lcd.print("Unregistered Card");
                fb->logRFIDAccess(cardUID, "Unknown", "denied");
                fb->logEvent("security", "Quét thẻ chưa đăng ký: " + cardUID);
            } else {
                lcd.print("Card Inactive");
                fb->logRFIDAccess(cardUID, holderName, "inactive_denied");
                fb->logEvent("security", "Quét thẻ bị khóa: " + holderName);
            }
            
            // Bắt đầu bộ đếm — update() tiếp theo sẽ gọi showStandbyMessage() sau 2.5s
            isDenying = true;
            denyStartTime = millis();
        }

        // Dừng đọc thẻ hiện tại
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }

    void openDoor() {
        if (!isOpen) {
            doorServo.write(DOOR_OPEN_ANGLE);
            isOpen = true;
            openTime = millis();
            fb->setString("/devices/door/status", "open");
            Serial.println("Door opened.");
        }
    }

    void closeDoor() {
        if (isOpen) {
            doorServo.write(DOOR_CLOSE_ANGLE);
            isOpen = false;
            fb->setString("/devices/door/status", "closed");
            Serial.println("Door closed.");
            showStandbyMessage();
        }
    }

    void showStandbyMessage() {
        if (WifiHelper::getAPMode()) {
            return;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   SMART HOME   ");
        lcd.setCursor(0, 1);
        lcd.print("Scan RFID Card..");
    }

    void setAutoCloseDelay(unsigned long ms) {
        autoCloseDelay = ms;
    }

    void setStatusFromFirebase(const String& status) {
        if (status == "open") {
            openDoor();
            
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Door Opened");
            lcd.setCursor(0, 1);
            lcd.print("via WebApp");
        } else if (status == "closed") {
            closeDoor();
        }
    }
};

#endif // DOOR_CONTROLLER_H
