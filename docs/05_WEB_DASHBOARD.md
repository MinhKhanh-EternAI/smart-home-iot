# 5. Web Dashboard — Cấu Trúc & Modules

## Kiến trúc

```
web-dashboard/
├── index.html            # HTML chính (612 dòng) — Glassmorphism UI
├── style.css             # Stylesheet (2929 dòng) — Dark/Light mode + responsive
├── app.js                # Entry point — Firebase init, routing, theme, toast
├── firebase-config.js    # Firebase API key & URL (gitignored)
├── firebase-config.example.js  # Mẫu để copy
├── CNAME                 # Domain mapping cho GitHub Pages
└── modules/
    ├── dashboard.js      # Tab Dashboard — sensors, devices, schedule, day picker
    ├── rfid.js           # Tab RFID — card list, add card, access logs
    ├── logs.js           # Tab Event Logs — filter system, sensor, automation, etc.
    └── config.js         # Tab Config — WiFi, MCP endpoint, Tools JSON editor
```

## Routing & Navigation

- **Sidebar** (desktop) và **Bottom Nav** (mobile) với các tab:
  - `dashboard` → `#dashboard` → `/`
  - `rfid` → `#rfid` → RFID Manager
  - `logs` → `#logs` → Event Logs
  - `config` → `#config` → System Config
- Tab được lưu trong URL hash, hỗ trợ browser back/forward (`popstate`)

## Firebase SDK (CDN)

```javascript
import { initializeApp } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-app.js";
import { getDatabase, ref, onValue, set, remove, query, limitToLast } from "firebase-database.js";
import { getAuth, signInAnonymously } from "firebase-auth.js";
```

## Module: dashboard.js (649 dòng)

### Chức năng
- **Sensors**: Hiển thị nhiệt độ, độ ẩm, trạng thái mưa, chuyển động từ `/sensors`
- **Devices**: Điều khiển bật/tắt, chế độ, hẹn giờ từ `/devices/{device}`
- **Schedule UI**: Time picker premium (cuộn chọn giờ/phút), day picker (bitmask)
- **Smart Alerts**: Cảnh báo khi trời mưa mà mái che đang mở thủ công

### Firebase listeners
```javascript
onValue(ref(db, "sensors"), ...)           // Cập nhật sensors
onValue(ref(db, "devices"), ...)           // Đồng bộ devices + schedule
onValue(ref(db, "devices/fan"), ...)       // Riêng fan
```

### Điều khiển
```javascript
set(ref(db, "devices/indoor_light/status"), true)   // Bật đèn
set(ref(db, "devices/roof/mode"), "auto")            // Đổi chế độ mái
set(ref(db, "devices/door/status"), "open")          // Mở cửa (có confirm)
```

## Module: rfid.js (135 dòng)

### Chức năng
- Load danh sách thẻ từ `rfid/cards` vào bảng
- Bật/tắt active status thẻ (checkbox)
- Xóa thẻ (có confirm modal)
- Form đăng ký thẻ mới
- Lịch sử quẹt thẻ từ `rfid/access_logs` (limitToLast 15)

## Module: logs.js (91 dòng)

### Chức năng
- Load event logs từ `logs/event_logs` (limitToLast 40)
- Bộ lọc: All, System, Sensor, Automation, Schedule, Security
- Mỗi log type có icon và màu sắc riêng

| Log Type | Icon | Màu border |
|----------|------|------------|
| `system` | fa-server | Indigo |
| `sensor` | fa-droplet | Sky Blue |
| `automation` | fa-robot | Emerald |
| `schedule` | fa-clock | Amber |
| `security` | fa-shield-halved | Rose |

## Module: config.js (86 dòng)

### Chức năng
- **WiFi Config**: Form SSID + Password, ghi vào `config/wifi`
- **MCP Endpoint**: Input URL, ghi vào `config/mcp/endpoint`
- **Tools JSON Editor**: Textarea monospace editor, validate JSON client-side, ghi vào `config/tools`

## CSS Theme System

- **Dark mode** (mặc định): CSS variables với `--bg-gradient: dark`, `--panel-bg: rgba(22, 28, 45, 0.45)`
- **Light mode**: Class `body.light-mode` override toàn bộ CSS variables, white cards, indigo accents
- **Theme lưu**: `localStorage.getItem("theme")`, mặc định `"light"`
- **Flash prevention**: Inline script trong `<head>` áp dụng class trước khi render

## Mobile Responsive
- **<768px**: Ẩn sidebar, hiện bottom nav, 2 cột sensors, 1 cột devices
- **<480px**: 1 cột sensors, filter bar xếp dọc
