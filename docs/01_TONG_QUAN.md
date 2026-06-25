# 1. Tổng Quan Kiến Trúc & Chức Năng

## Kiến trúc hệ thống

```
┌─────────────────────────────────────────────────────────────┐
│                         AI Agent                            │
│  (Claude/GPT via MCP hoặc opencode agent đọc docs trước)    │
└──────────────┬──────────────────────────────────────────────┘
               │ đọc docs / sinh code
               ▼
┌─────────────────────────────────────────────────────────────┐
│                    Web Dashboard                            │
│  (Vanilla JS + Firebase SDK + Glassmorphism CSS)            │
│  Deployed: iot.nguyenvanminhkhanh.id.vn                     │
└──────────────┬──────────────────────────────────────────────┘
               │ Firebase Realtime DB (đọc/ghi)
               ▼
┌─────────────────────────────────────────────────────────────┐
│              Firebase Realtime Database                      │
│  Rules: auth != null (anonymous sign-in)                    │
│  Regions: /devices, /sensors, /rfid, /logs, /config         │
└────┬──────────────────────┬─────────────────────────────────┘
     │ stream callback      │ HTTP REST
     ▼                      ▼
┌──────────────┐    ┌──────────────┐
│ ESP8266 Core  │    │ ESP32 (phụ)  │
│ (NodeMCU Lolin)│    │ (ESP32 Dev)  │
│ C++ Arduino   │    │ C++ Arduino  │
│ PlatformIO    │    │ PlatformIO   │
└──────┬───────┘    └──────┬───────┘
       │ GPIO/I2C/SPI/PWM  │ I2C/UART
       ▼                    ▼
  ┌─────────┐         ┌─────────┐
  │ Thiết bị │         │ Thiết bị │
  │ vật lý   │         │ vật lý   │
  └─────────┘         └─────────┘
```

## Luồng dữ liệu chính

```
Web Dashboard ──set──► Firebase ──stream──► ESP8266 ──GPIO──► Thiết bị
                         │                                         │
                         ├── /devices/{device}/status              │
                         ├── /sensors/{temperature,humidity,...}   │
                         ├── /rfid/{cards,access_logs}             │
                         └── /logs/event_logs                      │
                                                                   │
ESP8266 ──set──► Firebase ──onValue──► Web Dashboard (cập nhật UI)
```

## Chức năng đã triển khai

| Chức năng | ESP8266 | Web Dashboard | Ghi chú |
|-----------|---------|---------------|---------|
| Đọc nhiệt độ/độ ẩm DHT11 | ClimateController.h | dashboard.js | Cập nhật 10s, threshold 0.5°C/1% |
| Cảm biến mưa (Water Sensor) | RoofController.h | dashboard.js | A0, threshold <500=mưa, đọc 3s |
| Cảm biến PIR (chuyển động) | LightController.h | dashboard.js | PCF8574 P3 / D4, timeout 30s |
| Đèn trong nhà (Indoor Light) | LightController.h | dashboard.js | PCF8574 P1, schedule 7 ngày |
| Đèn ngoài sân (Outdoor Light) | LightController.h | dashboard.js | PCF8574 P0, auto (PIR) hoặc schedule |
| Quạt thông minh | LightController.h | dashboard.js | PCF8574 P4 |
| Cửa RFID (RC522 + Servo) | DoorController.h | rfid.js | SPI, verify Firebase, auto-close |
| Mái che (Servo + WaterSensor) | RoofController.h | dashboard.js | Auto đóng khi mưa / thủ công |
| Buzzer báo hiệu | LightController.h | — | PCF8574 P2, 1 bíp (granted), 3 bíp (denied) |
| LCD I2C 16x2 | WifiHelper.h / DoorController.h | — | I2C address 0x27/0x3F/0x20 |
| Event logs | FirebaseHelper.h | logs.js | 6 filter types, pushJSON |
| RFID access logs | FirebaseHelper.h | rfid.js | Lịch sử quẹt thẻ (15 gần nhất) |
| Cấu hình WiFi từ xa | WifiHelper.h | config.js | EEPROM, AP fallback |
| Cấu hình Tools JSON | — | config.js | Editor trên web, sync Firebase |
| Cấu hình MCP endpoint | — | config.js | Xiaozhi MCP endpoint |

## Chế độ hoạt động

**Indoor Light**: `manual` (bật/tắt thủ công) | `schedule` (theo lịch)
**Outdoor Light**: `auto` (PIR - tự động bật khi có người, tắt sau 30s) | `manual` (thủ công + có thể schedule)
**Roof**: `auto` (tự động đóng khi mưa, mở khi tạnh) | `manual` (bật/tắt từ web)
**Door**: Luôn thủ công (quẹt thẻ hoặc mở từ xa qua web)

## Hẹn giờ (Schedule)

- Mỗi thiết bị có: `on_time`, `off_time`, `days` (bitmask 7 ngày), `enabled`
- Bitmask: bit0=CN, bit1=T2, ..., bit6=T7, 127=tất cả các ngày
- ESP8266 kiểm tra mỗi 5 giây qua NTP time
- Chỉ trigger 1 lần/phút (tránh spam)
