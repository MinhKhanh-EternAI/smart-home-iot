# 6. Hạ Tầng — CI/CD, Firebase, Domain

## CI/CD — GitHub Actions

**File**: `.github/workflows/deploy.yml`

- Trigger: push lên nhánh `main`
- Deploy: `web-dashboard/` → **GitHub Pages**
- Tự động tạo `firebase-config.js` từ GitHub Secrets (`FIREBASE_API_KEY`, `FIREBASE_DATABASE_URL`)
- Domain: `iot.nguyenvanminhkhanh.id.vn`

## Firebase Console

### Authentication
- **Method**: Anonymous (không cần đăng nhập)
- **Mục đích**: Đáp ứng Security Rules (`auth != null`)
- **SDK**: `signInAnonymously(auth)`

### Realtime Database
- **Region**: `asia-southeast1` (Singapore)
- **URL Pattern**: `https://{PROJECT_ID}-default-rtdb.asia-southeast1.firebasedatabase.app/`
- **Rules**: Yêu cầu `auth != null` cho tất cả node (trừ root bị chặn)

### Firebase Config (bí mật — không commit)

**ESP8266**: `include/Secrets.h` (gitignored)
```cpp
#define FIREBASE_HOST    "PROJECT_ID-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_API_KEY "your_api_key"
```

**Web Dashboard**: `web-dashboard/firebase-config.js` (gitignored)
```javascript
export const firebaseConfig = {
    apiKey: "your_api_key",
    databaseURL: "https://PROJECT_ID-default-rtdb.asia-southeast1.firebasedatabase.app/"
};
```

## Domain & DNS

| Record | Giá trị |
|--------|---------|
| Domain | `iot.nguyenvanminhkhanh.id.vn` |
| Type | CNAME |
| Target | `<username>.github.io` (GitHub Pages) |
| File | `CNAME` (cả ở root repo và `web-dashboard/CNAME`) |

## Secrets cần thiết cho CI/CD

| Secret Name | Mô tả |
|-------------|-------|
| `FIREBASE_API_KEY` | API Key từ Firebase Console |
| `FIREBASE_DATABASE_URL` | Database URL (vd: `https://...firebasedatabase.app/`) |

## Development Local

### ESP8266
```bash
cd esp8266-core
platformio run --target upload
platformio device monitor --baud 115200
```

### Web Dashboard
```bash
cd web-dashboard
npx http-server -p 8080 -c-1
# Mở http://localhost:8080
```

### Cần tạo file local (từ example)
- `esp8266-core/include/Secrets.h` (từ `Secrets.h.example`)
- `web-dashboard/firebase-config.js` (từ `firebase-config.example.js`)

## MCP Integration (đã chuẩn bị)

Firebase đã có node `/config/mcp/endpoint` và `/config/tools` sẵn sàng cho việc kết nối AI agent qua MCP protocol. ESP8266 chưa xử lý node này — đây là phần để phát triển thêm.
