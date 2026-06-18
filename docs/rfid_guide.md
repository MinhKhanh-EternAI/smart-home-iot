# Hướng Dẫn Sử Dụng & Đấu Nối RFID RC522 (ESP8266 NodeMCU Lolin)

Tài liệu này lưu trữ thông tin chi tiết về sơ đồ đấu nối phần cứng của đầu đọc thẻ **RFID RC522** và quy trình từng bước để quét, lấy mã UID và đăng ký thẻ mới trên hệ thống Smart Home IoT.

---

## 1. Sơ Đồ Đấu Nối Phần Cứng (Wiring Pinout)

Đầu đọc thẻ RFID RC522 giao tiếp với ESP8266 thông qua giao thức truyền thông **SPI**. 
* **Lưu ý đặc biệt quan trọng**: Module RC522 hoạt động ở mức nguồn **3.3V**. Việc cắm nhầm vào chân 5V của ESP8266 sẽ gây hỏng chip thu phát RF của module đầu đọc ngay lập tức.

### Bảng kết nối chân (ESP8266 ➡️ RC522)

| Chân trên RC522 | Màu dây gợi ý | Chân trên ESP8266 | Tên chân GPIO | Chức năng (SPI Bus) | Ghi chú kỹ thuật |
| :---: | :---: | :---: | :---: | :---: | :--- |
| **SDA (SS)** | 🔴 Đỏ | **D8** | GPIO15 | Slave Select (SS) | Chân chọn thiết bị SPI. Cần kéo xuống đất lúc khởi động nhưng ESP8266 có sẵn trở kéo thích hợp trên board nên hoạt động bình thường. |
| **SCK** | 🟡 Vàng | **D5** | GPIO14 | Serial Clock (SCK) | Tạo xung nhịp truyền dữ liệu SPI. |
| **MOSI** | 🟢 Xanh lá | **D7** | GPIO13 | Master Out Slave In | Chân ESP truyền dữ liệu tới RC522. |
| **MISO** | 🔵 Xanh dung | **D6** | GPIO12 | Master In Slave Out | Chân ESP nhận dữ liệu từ RC522. |
| **IRQ** | - | *Bỏ trống* | - | Interrupt Request | Không sử dụng chân ngắt trong dự án này. |
| **GND** | ⚫ Đen | **GND** | GND | Ground | Nối đất (Chung cực âm nguồn). |
| **RST** | 🟤 Nâu | **D0** | GPIO16 | Reset | Chân thiết lập lại trạng thái RC522. |
| **3.3V** | 🟠 Cam | **3.3V** | 3.3V | VCC Power | **Cấp nguồn 3.3V** từ bo mạch ESP8266. |

---

## 2. Quy Trình Quét & Đăng Ký Thẻ RFID Mới

Khi bạn có một chiếc thẻ hoặc móc khóa RFID trắng (tần số 13.56 MHz), quy trình đăng ký diễn ra theo 3 bước sau:

### Bước 1: Quét thẻ thô để lấy mã định danh (UID)
Mỗi chiếc thẻ RFID khi sản xuất ra đều có một mã định danh duy nhất (UID) gồm 8 ký tự Hex (ví dụ: `A1B2C3D4`). Bạn lấy mã này bằng 2 cách sau:

*   **Cách 1: Xem qua Serial Monitor (Lập trình viên)**
    1. Cắm ESP8266 vào máy tính qua cổng USB.
    2. Mở cửa sổ **Serial Monitor** trên phần mềm VS Code (PlatformIO) ở tốc độ baud **`115200`**.
    3. Đưa thẻ lại gần module RC522.
    4. Màn hình Serial sẽ hiện thông báo: `RFID Scanned! UID: A1B2C3D4`. Hãy copy mã này.
*   **Cách 2: Quét thẻ và xem trên Web Dashboard (Tiện lợi nhất)**
    1. Bật nguồn thiết bị và đảm bảo thiết bị đã kết nối Wi-Fi & Firebase (Đèn DB Status trên Web sáng xanh lá).
    2. Đưa chiếc thẻ mới vào quét trên đầu đọc RC522. 
    3. Vì thẻ chưa đăng ký, thiết bị sẽ từ chối mở cửa và lập tức ghi nhận cảnh báo lên hệ thống.
    4. Bạn mở trình duyệt Web Dashboard, vào tab **Event Logs (Nhật ký hoạt động)**. 
    5. Tại đây, bạn sẽ thấy một log bảo mật có màu đỏ dạng: `Quét thẻ chưa đăng ký: A1B2C3D4`. Hãy copy lấy mã UID này.

### Bước 2: Nhập thông tin đăng ký trên Web
1. Tại Web Dashboard (`http://localhost:8080`), điều hướng sang tab **RFID Manager** (Quản lý thẻ RFID).
2. Tại mục **Đăng ký thẻ mới** ở khung bên phải:
   *   **Mã thẻ UID**: Dán (hoặc gõ) mã UID lấy được ở Bước 1 vào ô này.
   *   **Tên người sở hữu**: Nhập tên của người sử dụng thẻ (Ví dụ: `Bố`, `Mẹ`, `Khách VIP`,...).
3. Nhấp nút **Đăng ký thẻ**.
4. Hệ thống sẽ hiển thị một thông báo Toast thông báo thành công và thẻ mới sẽ ngay lập tức được thêm vào bảng danh sách thẻ ở khung bên trái.

---

## 3. Trạng Thái Hoạt Động Của Hệ Thống Khi Quét Thẻ

Sau khi nạp thẻ thành công, mỗi lần quét thẻ hệ thống sẽ phản hồi như sau:

### 🟢 Khi Quét Thẻ Hợp Lệ (Đã đăng ký và đang hoạt động)
*   **Trên Thiết bị**:
    *   Màn hình LCD hiển thị dòng chữ: `Access GRANTED` ở dòng 1 và `[Tên người dùng]` ở dòng 2.
    *   Còi chíp (Buzzer) kêu **1 tiếng bíp ngắn**.
    *   Servo Cửa xoay một góc 90 độ để mở cửa, duy trì trong khoảng thời gian tự đóng (mặc định 15s) rồi tự xoay về góc 0 độ để đóng cửa.
*   **Trên Giao diện Web**:
    *   Hiện thông báo Toast màu xanh lá: `Mở cửa thành công cho [Tên người dùng]`.
    *   Đồng bộ trạng thái biểu tượng ổ khóa Cửa thành "Mở" màu xanh.
    *   Lịch sử ra vào ghi nhận lượt quét thẻ có tag màu xanh lá: `Hợp lệ`.

### 🔴 Khi Quét Thẻ Không Hợp Lệ (Thẻ bị khóa hoặc chưa đăng ký)
*   **Trên Thiết bị**:
    *   Màn hình LCD hiển thị dòng chữ: `Access DENIED!` ở dòng 1 và lý do (`Unregistered Card` - thẻ chưa đăng ký hoặc `Card Inactive` - thẻ bị khóa) ở dòng 2.
    *   Còi chíp (Buzzer) kêu **3 tiếng bíp ngắn, dồn dập**.
    *   Servo cửa đứng im (Cửa khóa).
*   **Trên Giao diện Web**:
    *   Lịch sử ra vào ghi nhận lượt quét có tag màu đỏ: `Từ chối` hoặc `Bị khóa`.
    *   Tab **Event Logs** xuất hiện dòng cảnh báo bảo mật tương ứng.

---

## 4. Xử Lý Sự Cố Thường Gặp (Troubleshooting)

1.  **Đưa thẻ vào đầu đọc nhưng LCD và Serial Monitor hoàn toàn không phản ứng gì?**
    *   *Kiểm tra vật lý*: Đảm bảo chân **RST** (Reset) đã được nối vào chân **D0** và chân **SDA** đã được nối vào chân **D8** của ESP.
    *   *Nguồn cấp*: Đo lại chân nguồn của RC522 xem có đúng là 3.3V không. Nếu nguồn yếu hoặc sụt áp do Servo/Relay chạy chung nguồn, RC522 sẽ ngừng hoạt động.
2.  **Đọc được thẻ nhưng báo lỗi "Communication Error" trên Serial?**
    *   Khoảng cách dây SPI quá dài (khuyên dùng dây dưới 20cm) hoặc dây bị lỏng, tiếp xúc kém làm suy hao tín hiệu tần số cao của bus SPI.
