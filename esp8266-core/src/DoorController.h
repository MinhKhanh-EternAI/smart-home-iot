#ifndef DOOR_CONTROLLER_H
#define DOOR_CONTROLLER_H

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include "Config.h"
#include "FirebaseHelper.h"
#include "WifiHelper.h"

class DoorController {
private:
    MFRC522 mfrc522;
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
        mfrc522(RC522_SS_PIN, RC522_RST_PIN)
    {}

    void init(FirebaseHelper* fbHelper) {
        fb = fbHelper;
        
        // Khởi tạo SPI và RFID
        SPI.begin();
        mfrc522.PCD_Init();
        Serial.println("RFID RC522 Initialized.");

        // Hiển thị thông báo chờ (LCD tự init qua WifiHelper::printToLCD)
        if (!WifiHelper::getAPMode()) {
            showStandbyMessage();
            Serial.println("LCD I2C ready.");
        } else {
            Serial.println("LCD I2C skip (AP Mode).");
        }

        doorServo.attach(SERVO_DOOR_PIN);
        doorServo.write(DOOR_CLOSE_ANGLE);
        Serial.printf("Servo Door attached to Pin: %d\n", SERVO_DOOR_PIN);
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

        WifiHelper::printToLCD("Checking card...", "");
        
        String holderName = "Unknown";
        bool isActive = false;

        // Xác thực với Firebase
        bool cardExists = fb->checkRFIDCard(cardUID, holderName, isActive);

        if (cardExists && isActive) {
            // Xác thực thành công
            Serial.printf("Access GRANTED to: %s\n", holderName.c_str());
            
            WifiHelper::printToLCD("Access GRANTED", holderName.substring(0, 16));

            fb->logRFIDAccess(cardUID, holderName, "granted");
            fb->logEvent("door", "Cửa mở bằng thẻ RFID: " + holderName);
            
            openDoor();
        } else {
            // Xác thực thất bại
            Serial.println("Access DENIED!");
            
            if (!cardExists) {
                WifiHelper::printToLCD("Access DENIED!", "Unregistered Card");
                fb->logRFIDAccess(cardUID, "Unknown", "denied");
                fb->logEvent("security", "Quét thẻ chưa đăng ký: " + cardUID);
            } else {
                WifiHelper::printToLCD("Access DENIED!", "Card Inactive");
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
            WifiHelper::showNotification("Cua thong minh", "DA MO");
        }
    }

    void closeDoor() {
        if (isOpen) {
            doorServo.write(DOOR_CLOSE_ANGLE);
            isOpen = false;
            fb->setString("/devices/door/status", "closed");
            Serial.println("Door closed.");
            WifiHelper::showNotification("Cua thong minh", "DA DONG");
        }
    }

    void showStandbyMessage() {
        WifiHelper::showDefaultScreen();
    }

    void setAutoCloseDelay(unsigned long ms) {
        autoCloseDelay = ms;
    }

};

#endif // DOOR_CONTROLLER_H
