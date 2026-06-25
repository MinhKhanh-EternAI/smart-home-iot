// ==========================================================
// BÍ MẬT FIREBASE — KHÔNG commit firebase-config.js lên git!
//
// Hướng dẫn chạy local:
//   1. Sao chép file này thành  web-dashboard/firebase-config.js
//   2. Điền thông tin Firebase dự án của bạn vào bên dưới
//   3. Kiểm tra .gitignore đã có dòng "web-dashboard/firebase-config.js"
//
// Lấy thông tin tại: Firebase Console → Project Settings → General → firebaseConfig
//
// Khi deploy qua GitHub Actions:
//   File này được tạo tự động từ GitHub Secrets:
//     FIREBASE_API_KEY và FIREBASE_DATABASE_URL
//   Thêm 2 secrets đó tại: repo → Settings → Secrets and variables → Actions
// ==========================================================

export const firebaseConfig = {
    apiKey: "YOUR_FIREBASE_API_KEY",
    databaseURL: "https://YOUR_PROJECT_ID-default-rtdb.asia-southeast1.firebasedatabase.app/"
};

export const DEFAULT_AP_IP = "192.168.1.86";

