# Hướng Dẫn Thiết Lập Dự Án Fire Alarm ESP8266

Hướng dẫn này cung cấp các bước để thiết lập môi trường phát triển và sao chép repository cho dự án Fire Alarm ESP8266.

## 1. Thiết Lập Toolchain
- **Tải toolchain**: Tải tệp toolchain ESP32 cho Windows từ [liên kết này](https://dl.espressif.com/dl/esp32_win32_msys2_environment_and_toolchain-20181001.zip).
- **Giải nén tệp**: Giải nén tệp đã tải về vào vị trí bạn muốn (ví dụ: `C:\`). Sau khi giải nén, bạn sẽ thấy một thư mục có tên `msys32`.

## 2. Sao Chép Repository
- **Di chuyển đến thư mục `/home`**: Mở một terminal (không phải terminal `msys32`) và đi đến thư mục `/home`.
- **Sao chép repository**: Chạy lệnh sau để sao chép repository vào thư mục mang tên `<tên_người_dùng>`:
  ```bash
  git clone https://github.com/quangnm145/fire_alarm_esp8266.git ./<tên_người_dùng>/
  ```
- Thay <tên_người_dùng> bằng tên người dùng thực tế của bạn.
## 3. Chạy Môi Trường MSYS2
- Di chuyển đến thư mục msys32: Đi đến thư mục msys32 đã giải nén.
- Khởi động terminal: Nhấp đúp vào tệp mingw32.exe (biểu tượng màu xám) để mở terminal MSYS2.
