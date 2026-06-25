# Hệ Thống Nhà Thông Minh - Smart Home IoT 🏠✨

Dự án nghiên cứu và triển khai hệ thống **Smart Home IoT (Phase 1)** dành cho học phần IoT (Năm 4). Hệ thống sử dụng vi điều khiển chính **ESP8266** kết nối với cảm biến và cơ cấu chấp hành, giao tiếp thời gian thực với **Firebase Realtime Database** và được quản lý trực quan qua **Web Dashboard** hiện đại.

---

## 🌐 Đường Dẫn Triển Khai (Deployed Domain)
*   **Trang Dashboard quản trị:** [https://iot.nguyenvanminhkhanh.id.vn](https://iot.nguyenvanminhkhanh.id.vn)
*   *Hệ thống được deploy tự động (CI/CD) qua GitHub Actions mỗi khi push code lên nhánh `main`.*

---

## 📁 Cấu Trúc Thư Mục Dự Án (Project Directory Structure)

```text
smart-home-iot/
├── .github/workflows/   
│   └── deploy.yml              # Quy trình CI/CD tự động deploy thư mục web-dashboard lên GitHub Pages
├── docs/                       # Tài liệu hướng dẫn chi tiết
│   ├── hardware_wiring.md      # Hướng dẫn đấu nối phần cứng, GPIO mapping và sơ đồ nguồn
│   └── rfid_guide.md           # Hướng dẫn quét, lấy mã UID và đăng ký thẻ RFID
├── esp8266-core/               # Mã nguồn C++ điều khiển thiết bị chính (ESP8266 NodeMCU Lolin)
├── esp32-assistant/            # Mã nguồn mở rộng phụ trợ (ESP32)
├── firebase-schema/            # Cấu trúc cơ sở dữ liệu mẫu Firebase Realtime Database
├── web-dashboard/              # Mã nguồn giao diện Web Portal quản trị
│   ├── index.html              # Trang chủ giao diện Web (Thiết kế Glassmorphism)
│   ├── style.css               # Hệ thống Stylesheets hỗ trợ giao diện Sáng / Tối (Light/Dark mode)
│   ├── app.js                  # Điểm khởi tạo Firebase SDK và xử lý điều hướng chính
│   ├── CNAME                   # Khai báo tên miền tùy chỉnh cho GitHub Pages
│   └── modules/                # Các module logic của Dashboard (Dashboard, RFID, Logs, Config)
└── CNAME                       # Tên miền tùy chỉnh liên kết ở root của repository
```

---

## 🚀 Các Tính Năng Chính (Key Features)

1.  **Giám sát Môi trường Thời gian thực (Real-time Environment Monitoring):**
    *   Cảm biến **DHT11**: Đo và hiển thị liên tục Nhiệt độ và Độ ẩm trong nhà.
    *   Cảm biến **Nước/Mưa (Water Sensor)**: Phát hiện trạng thái mưa thời gian thực.
2.  **Cửa Thông minh RFID RC522 (Smart Door Authentication):**
    *   Xác thực thẻ RFID tần số 13.56 MHz (Mở cửa bằng Servo MG90S khi quẹt thẻ hợp lệ, kêu **1 bíp**).
    *   Từ chối truy cập và báo còi **3 bíp dồn dập** nếu quẹt thẻ chưa đăng ký hoặc bị khóa.
    *   Quản lý danh sách thẻ (Đăng ký mới, Bật/Tắt trạng thái hoạt động, Xóa thẻ) trực tiếp từ Web Dashboard.
3.  **Tự động hóa & Hẹn giờ (Automation & Scheduler):**
    *   **Tự động đóng/mở mái che (Smart Roof)** bằng Servo dựa trên trạng thái cảm biến mưa (Tự đóng khi có mưa).
    *   **Tự động bật đèn sân vườn (Outdoor Light)** vào buổi tối (sử dụng cảm biến ánh sáng hoặc logic hẹn giờ).
    *   Hẹn giờ (Schedule) chi tiết đến từng phút để bật/tắt thiết bị (Đèn trong nhà, Đèn ngoài sân, Quạt gió).
4.  **Nhật ký Hoạt động thông minh (Event Logs Feed):**
    *   Bộ lọc đa kênh: Tất cả, Hệ thống, Cảm biến, Tự động hóa, Hẹn giờ, Bảo mật.
    *   Lưu trữ lịch sử quẹt thẻ RFID thời gian thực với tên người sở hữu và kết quả xác thực.
5.  **Cấu hình Trực quan trên Web (System Configuration):**
    *   Đổi thông tin kết nối WiFi trực tiếp từ xa để thiết bị tự cấu hình lại.
    *   Lưu trữ và chỉnh sửa cấu hình Tools JSON trực tiếp trên Web đồng bộ hóa với Firebase.

---

## 🔌 Đấu Nối Phần Cứng (Hardware Wiring)

Chi tiết sơ đồ chân GPIO của **ESP8266** và module mở rộng I/O **PCF8574** xem tại tài liệu chuyên biệt:  
👉 [docs/hardware_wiring.md](file:///c:/Users/khanhnvm/Documents/workspace/uni/year4/iot/smart-home-iot/docs/hardware_wiring.md)

### Tóm tắt kết nối chính:
*   **RFID RC522:** Giao tiếp qua giao thức SPI (D5, D6, D7, D8, D0).
*   **Màn hình LCD I2C & PCF8574:** Chia sẻ chung bus I2C (SDA -> D2, SCL -> D1).
*   **Relays điều khiển thiết bị (Đèn, Quạt) & Buzzer:** Điều khiển gián tiếp thông qua các chân đầu ra trên module **PCF8574** để tiết kiệm chân GPIO của ESP8266.
*   **Servo Cửa (Door) & Mái che (Roof):** Nối tương ứng vào chân **D4 (GPIO2)** và **RX (GPIO3)** của ESP8266.

---

## 💳 Quy Trình Đăng Ký Thẻ RFID Mới

Xem hướng dẫn chi tiết quy trình lấy mã UID thẻ và phân quyền truy cập:  
👉 [docs/rfid_guide.md](file:///c:/Users/khanhnvm/Documents/workspace/uni/year4/iot/smart-home-iot/docs/rfid_guide.md)

---

## 🛠️ Hướng Dẫn Chạy & Phát Triển Cục Bộ (Local Development)

### Cấu hình Firebase
Nếu muốn chạy với cơ sở dữ liệu Firebase của riêng bạn:
1.  Truy cập Firebase Console, tạo dự án và bật dịch vụ **Realtime Database**.
2.  Copy thông số cấu hình Firebase (API Key, Database URL) thay thế vào đoạn mã cấu hình trong file [web-dashboard/app.js](file:///c:/Users/khanhnvm/Documents/workspace/uni/year4/iot/smart-home-iot/web-dashboard/app.js#L13-L16):
    ```javascript
    const firebaseConfig = {
        apiKey: "YOUR_API_KEY",
        databaseURL: "YOUR_DATABASE_URL"
    };
    ```

### Chạy giao diện Web cục bộ (Local Run)
1.  Sử dụng tiện ích mở rộng **Live Server** trên VS Code hoặc cài đặt công cụ http-server thông qua Node.js:
    ```bash
    npm install -g http-server
    cd web-dashboard
    http-server -p 8080
    ```
2.  Mở trình duyệt truy cập địa chỉ `http://localhost:8080` để trải nghiệm giao diện Dashboard.
