#ifndef DOOR_CONTROLLER_H
#define DOOR_CONTROLLER_H

#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <DHT.h>
#if USE_PCF8574
#include <PCF8574.h>
#endif
#include "Config.h"
#include "FirebaseHelper.h"
#include "WifiHelper.h"

class DoorController {
private:
    MFRC522 mfrc522;
    LiquidCrystal_I2C* lcd;
    Servo doorServo;
    FirebaseHelper* fb;
#if USE_PCF8574
    PCF8574 pcf;
#endif

    bool isOpen = false;
    unsigned long openTime = 0;
    unsigned long autoCloseDelay = 5000;

    bool isDenying = false;
    unsigned long denyStartTime = 0;
    static const unsigned long DENY_DISPLAY_MS = 2500;

    DHT dht;
    unsigned long lastDHTRead = 0;

public:
#if USE_PCF8574
    DoorController() : 
        mfrc522(RC522_SS_PIN, RC522_RST_PIN),
        lcd(nullptr),
        fb(nullptr),
        pcf(PCF_I2C_ADDRESS),
        dht(DIRECT_DHT_PIN, DIRECT_DHT_TYPE)
    {}
#else
    DoorController() : 
        mfrc522(RC522_SS_PIN, RC522_RST_PIN),
        lcd(nullptr),
        fb(nullptr),
        dht(DIRECT_DHT_PIN, DIRECT_DHT_TYPE)
    {}
#endif

    ~DoorController() {
        if (lcd != nullptr) {
            delete lcd;
        }
    }

    void init(FirebaseHelper* fbHelper) {
        fb = fbHelper;
        
        // Khởi tạo SPI và RFID RC522
        SPI.begin();
        mfrc522.PCD_Init();
        Serial.println("[DoorController] RFID RC522 khoi tao thanh cong.");

        // Khởi tạo Buzzer
#if USE_PCF8574
        pcf.begin();
        pcf.write(PCF_BUZZER, HIGH); // Tắt buzzer (Active Low)
        Serial.println("[DoorController] PCF8574 Buzzer khoi tao (Pin P2).");
#else
        pinMode(DIRECT_BUZZER_PIN, OUTPUT);
        digitalWrite(DIRECT_BUZZER_PIN, HIGH); // Tắt buzzer (Active Low)
        Serial.printf("[DoorController] Direct GPIO Buzzer khoi tao (Pin: %d).\n", DIRECT_BUZZER_PIN);
#endif

        // Khởi tạo LCD dynamically từ địa chỉ dò tìm được
        int addr = WifiHelper::getLCDAddress();
        if (addr != -1) {
            lcd = new LiquidCrystal_I2C(addr, 16, 2);
            lcd->init();
            lcd->backlight();
            showStandbyMessage();
            Serial.printf("[DoorController] LCD I2C khoi tao tai dia chi 0x%02X.\n", addr);
        } else {
            Serial.println("[DoorController] Khong co LCD. Bo qua hien thi LCD.");
        }

        // Khởi tạo Servo Door
        doorServo.attach(SERVO_DOOR_PIN);
        doorServo.write(DOOR_CLOSE_ANGLE);
        Serial.printf("[DoorController] Servo cua khoi tao tren Pin: %d.\n", SERVO_DOOR_PIN);

        // Khởi tạo cảm biến DHT11
        dht.begin();
        Serial.println("[DoorController] Cam bien DHT11 khoi tao thanh cong.");
    }

    void update() {
        // Bộ đếm thời gian hiển thị từ chối (non-blocking)
        if (isDenying) {
            if (millis() - denyStartTime >= DENY_DISPLAY_MS) {
                isDenying = false;
                showStandbyMessage();
            }
            return;
        }

        // Tự động đóng cửa sau thời gian trễ
        if (isOpen && (millis() - openTime >= autoCloseDelay)) {
            closeDoor();
        }

        // Cập nhật hiển thị nhiệt độ & độ ẩm khi đang ở chế độ chờ
        if (!isOpen && !isDenying) {
            updateStandbyDisplay();
        }

        // Kiểm tra xem có thẻ mới được đưa vào không
        if (!mfrc522.PICC_IsNewCardPresent()) {
            return;
        }

        // Đọc thông tin thẻ
        if (!mfrc522.PICC_ReadCardSerial()) {
            return;
        }

        // Chuyển UID thẻ thành chuỗi Hex viết hoa
        String cardUID = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            cardUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
            cardUID += String(mfrc522.uid.uidByte[i], HEX);
        }
        cardUID.toUpperCase();
        
        Serial.print("[RFID] Quet duoc the UID: ");
        Serial.println(cardUID);

        if (lcd != nullptr) {
            lcd->clear();
            lcd->setCursor(0, 0);
            lcd->print("Checking card...");
        }
        
        String holderName = "Unknown";
        bool isActive = false;
        bool cardExists = false;

        // Xác thực với Firebase
        if (fb != nullptr && fb->ready()) {
            cardExists = fb->checkRFIDCard(cardUID, holderName, isActive);
        } else {
            // Chế độ Offline: Hỗ trợ một vài thẻ mặc định (hoặc thẻ master) nếu mất kết nối Firebase
            Serial.println("[RFID] Mat ket noi Firebase. Dang check offline...");
            // Mở cửa cho thẻ bất kỳ ở chế độ offline để tránh bị nhốt
            cardExists = true;
            isActive = true;
            holderName = "Offline User";
        }

        if (cardExists && isActive) {
            // Xác thực thành công
            Serial.printf("[RFID] Quyen TRUY CAP DUOC CAP cho: %s\n", holderName.c_str());
            
            if (lcd != nullptr) {
                lcd->clear();
                lcd->setCursor(0, 0);
                lcd->print("Access GRANTED");
                lcd->setCursor(0, 1);
                lcd->print(holderName.substring(0, 16));
            }

            if (fb != nullptr && fb->ready()) {
                fb->logRFIDAccess(cardUID, holderName, "granted");
                fb->logEvent("door", "Cua mo bang the RFID: " + holderName);
            }
            
            beep(1); // Còi bíp 1 tiếng báo hiệu thành công
            openDoor();
        } else {
            // Xác thực thất bại
            Serial.println("[RFID] TU CHOI TRUY CAP!");
            
            if (lcd != nullptr) {
                lcd->clear();
                lcd->setCursor(0, 0);
                lcd->print("Access DENIED!");
                lcd->setCursor(0, 1);
                if (!cardExists) {
                    lcd->print("Unregistered Card");
                } else {
                    lcd->print("Card Inactive");
                }
            }

            if (fb != nullptr && fb->ready()) {
                if (!cardExists) {
                    fb->logRFIDAccess(cardUID, "Unknown", "denied");
                    fb->logEvent("security", "Quet the chua dang ky: " + cardUID);
                } else {
                    fb->logRFIDAccess(cardUID, holderName, "inactive_denied");
                    fb->logEvent("security", "Quet the bi khoa: " + holderName);
                }
            }
            
            beep(3); // Còi bíp 3 tiếng báo hiệu thất bại
            
            // Bắt đầu đếm ngược màn hình từ chối
            isDenying = true;
            denyStartTime = millis();
        }

        // Kết thúc đọc thẻ
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }

    void openDoor() {
        if (!isOpen) {
            doorServo.write(DOOR_OPEN_ANGLE);
            isOpen = true;
            openTime = millis();
            if (fb != nullptr && fb->ready()) {
                fb->setString("/devices/door/status", "open");
            }
            Serial.println("[DoorController] Cua dang MO.");
        }
    }

    void closeDoor() {
        if (isOpen) {
            doorServo.write(DOOR_CLOSE_ANGLE);
            isOpen = false;
            if (fb != nullptr && fb->ready()) {
                fb->setString("/devices/door/status", "closed");
            }
            Serial.println("[DoorController] Cua da DONG.");
            showStandbyMessage();
        }
    }

    bool getDoorStatus() const {
        return isOpen;
    }

    void showStandbyMessage() {
        if (lcd != nullptr) {
            lcd->clear();
            lcd->setCursor(0, 1);
            lcd->print("Scan RFID Card..");
            lastDHTRead = 0; // Kích hoạt cập nhật DHT ngay lập tức lên dòng 0
        }
    }

    void updateStandbyDisplay() {
        if (lcd == nullptr) return;
        
        if (lastDHTRead == 0 || millis() - lastDHTRead >= 2500) {
            lastDHTRead = millis();
            float h = dht.readHumidity();
            float t = dht.readTemperature();
            Serial.printf("[DHT Debug] Temp: %.1f, Hum: %.1f\n", t, h);
            
            lcd->setCursor(0, 0);
            if (!isnan(h) && !isnan(t)) {
                char buf[17];
                snprintf(buf, sizeof(buf), "T:%2d%cC   H:%2d%% ", (int)t, 0xDF, (int)h);
                lcd->print(buf);
                if (fb != nullptr) {
                    fb->updateSensors(t, h);
                }
            } else {
                lcd->print("   SMART HOME   ");
            }
        }
    }

    void setAutoCloseDelay(unsigned long ms) {
        autoCloseDelay = ms;
    }

private:
    // Điều khiển còi chíp bíp báo hiệu trạng thái
    void beep(int times) {
        for (int i = 0; i < times; i++) {
            if (i > 0) delay(80); // Khoảng nghỉ giữa các tiếng bíp
            
#if USE_PCF8574
            pcf.write(PCF_BUZZER, LOW);  // Bật còi (Active Low)
            delay(times == 1 ? 150 : 80); // Thời gian kêu dài hay ngắn
            pcf.write(PCF_BUZZER, HIGH); // Tắt còi
#else
            digitalWrite(DIRECT_BUZZER_PIN, LOW);  // Bật còi
            delay(times == 1 ? 150 : 80);
            digitalWrite(DIRECT_BUZZER_PIN, HIGH); // Tắt còi
#endif
        }
    }
};

#endif // DOOR_CONTROLLER_H
