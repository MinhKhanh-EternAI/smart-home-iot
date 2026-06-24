# 3. Firebase Realtime Database Schema

## Cấu hình bảo mật

**Authentication**: Anonymous sign-in (yêu cầu `auth != null` cho tất cả node)
**Rules File**: `firebase-schema/database.rules.json`

```
{
  "rules": {
    ".read": false, ".write": false,  // chặn toàn bộ
    "devices":   { ".read": "auth != null", ".write": "auth != null" },
    "sensors":   { ".read": "auth != null", ".write": "auth != null" },
    "logs/event_logs": { ".read": "auth != null", ".write": "auth != null" },
    "rfid/cards":       { ".read": "auth != null", ".write": "auth != null" },
    "rfid/access_logs": { ".read": "auth != null", ".write": "auth != null" },
    "config":   { ".read": "auth != null", ".write": "auth != null" }
  }
}
```

## Cấu trúc dữ liệu đầy đủ

```json
{
  "config": {
    "wifi": {
      "ssid": "Ten_WiFi",
      "password": "MatKhauWiFi"
    },
    "mcp": {
      "endpoint": "http://your-xiaozhi-mcp-endpoint/api"
    },
    "tools": [
      {
        "name": "indoor_light",
        "desc": "Đèn trong nhà",
        "pin": "PCF8574_P1"
      },
      {
        "name": "outdoor_light",
        "desc": "Đèn sân vườn",
        "pin": "PCF8574_P0"
      },
      {
        "name": "smart_door",
        "desc": "Cửa tự động Servo",
        "pin": "RX"
      },
      {
        "name": "smart_roof",
        "desc": "Mái che tự động",
        "pin": "TX"
      }
    ]
  },

  "devices": {
    "indoor_light": {
      "status": false,
      "mode": "manual",
      "schedule": {
        "enabled": false,
        "on_time": "18:00",
        "off_time": "06:00",
        "days": 127
      }
    },
    "outdoor_light": {
      "status": false,
      "mode": "auto"
    },
    "door": {
      "status": "closed",
      "auto_close_ms": 5000
    },
    "roof": {
      "status": "open",
      "mode": "auto"
    },
    "fan": false
  },

  "sensors": {
    "temperature": 25.0,
    "humidity": 60.0,
    "rain": false,
    "motion": false,
    "last_updated": 1718496000
  },

  "rfid": {
    "cards": {
      "A1B2C3D4": {
        "name": "Nguyễn Văn A",
        "active": true,
        "created_at": 1718496000000
      }
    },
    "access_logs": {
      "-log_id_1": {
        "timestamp": 1718496000000,
        "card_uid": "A1B2C3D4",
        "name": "Nguyễn Văn A",
        "status": "granted"
      }
    }
  },

  "logs": {
    "event_logs": {
      "-event_id_1": {
        "timestamp": 1718496000000,
        "type": "system",
        "message": "Hệ thống khởi động thành công"
      }
    }
  }
}
```

## Chi tiết từng node

### `/devices/{device_name}`

| Field | Type | Values | Ghi chú |
|-------|------|--------|---------|
| `status` | bool / string | `true/false` hoặc `"open"/"closed"` | Trạng thái hiện tại |
| `mode` | string | `"manual"`, `"auto"`, `"schedule"` | Chế độ hoạt động |
| `schedule/enabled` | bool | `true/false` | Bật/tắt lịch trình |
| `schedule/on_time` | string | `"HH:MM"` | Giờ bật (24h) |
| `schedule/off_time` | string | `"HH:MM"` | Giờ tắt (24h) |
| `schedule/days` | int (bitmask) | 0-127 | bit0=CN, bit1=T2...bit6=T7 |
| `auto_close_ms` | int | ms | Thời gian tự đóng cửa |

### `/sensors`

| Field | Type | Ghi chú |
|-------|------|---------|
| `temperature` | float | °C, cập nhật khi thay đổi >=0.5°C |
| `humidity` | float | %, cập nhật khi thay đổi >=1% |
| `rain` | bool | `true`=có mưa (A0 < 500) |
| `motion` | bool | `true`=có người (PIR) |
| `last_updated` | int | Unix timestamp |

### `/rfid/cards/{uid}`

| Field | Type | Ghi chú |
|-------|------|---------|
| `name` | string | Tên chủ thẻ |
| `active` | bool | `true`=đang hoạt động, `false`=bị khóa |
| `created_at` | int | Unix epoch ms |

### `/rfid/access_logs`

| Field | Type | Values |
|-------|------|--------|
| `timestamp` | int | Unix epoch ms |
| `card_uid` | string | UID thẻ |
| `name` | string | Tên chủ thẻ |
| `status` | string | `"granted"`, `"denied"`, `"inactive_denied"` |

### `/logs/event_logs`

| Field | Type | Values |
|-------|------|--------|
| `timestamp` | int | Unix epoch ms |
| `type` | string | `"system"`, `"sensor"`, `"automation"`, `"schedule"`, `"security"` |
| `message` | string | Nội dung log |

## Web Dashboard → Firebase operations

| Module | Path | Method |
|--------|------|--------|
| dashboard | `devices/{device}/status` | set / onValue |
| dashboard | `devices/{device}/mode` | set / onValue |
| dashboard | `devices/{device}/schedule/*` | set / onValue |
| dashboard | `sensors/*` | onValue |
| dashboard | `devices/fan` | set / onValue |
| rfid | `rfid/cards/{uid}` | set / remove / onValue |
| rfid | `rfid/access_logs` | onValue (limitToLast 15) |
| logs | `logs/event_logs` | onValue (limitToLast 40) |
| config | `config/wifi` | set / onValue |
| config | `config/mcp` | set / onValue |
| config | `config/tools` | set / remove / onValue |

## ESP8266 → Firebase operations

| Chức năng | Path | Method |
|-----------|------|--------|
| Cập nhật sensors | `sensors/temperature` | setFloat |
| Cập nhật sensors | `sensors/humidity` | setFloat |
| Cập nhật sensors | `sensors/rain` | setBool |
| Cập nhật sensors | `sensors/motion` | setBool |
| Cập nhật sensors | `sensors/last_updated` | setInt |
| Đồng bộ thiết bị | `devices/{device}/status` | setBool / setString |
| Xác thực RFID | `rfid/cards/{uid}` | getJSON |
| Log sự kiện | `logs/event_logs` | pushJSON |
| Log RFID | `rfid/access_logs` | pushJSON |
| Stream điều khiển | `devices` | beginStream (callback → onDeviceControl) |

### Firebase Stream

ESP8266 lắng nghe **stream** trên node `/devices`. Khi Web Dashboard ghi vào đây, ESP8266 nhận callback `onDeviceControl(device, key, value)` và điều khiển thiết bị tương ứng.

Ví dụ: Web set `devices/indoor_light/status = true` → Stream callback → `onDeviceControl("indoor_light", "status", "true")` → `lightCtrl.setIndoorLight(true)` → PCF8574 P1 = LOW → Relay bật → Đèn sáng.
