# TEST_COMMIT_5 – Activity Logger & Basic Tests

## 1. Thông tin

- Dự án: Ứng dụng đấu giá trực tuyến bằng ngôn ngữ C.
- Phạm vi: Commit 5 – Ghi nhật ký hoạt động và kiểm thử cơ bản.
- Server: TCP socket, xử lý nhiều client bằng `select()`.
- File log: `auction_server.log` (được tạo tại thư mục làm việc khi chạy server).
- Bộ kiểm thử tự động: `test_commit5.py` (chỉ sử dụng thư viện chuẩn Python).

## 2. Nội dung đã bổ sung

### Activity Logger

Server ghi các sự kiện quan trọng vào `auction_server.log`, kèm thời gian:

- `SERVER_START`, `SERVER_STOP`.
- `CLIENT_CONNECTED`, `CLIENT_DISCONNECTED`.
- `COMMAND_RECEIVED` (chỉ ghi tên lệnh, không ghi toàn bộ nội dung lệnh).
- `REGISTER_SUCCESS`, `LOGIN_SUCCESS`, `LOGOUT`.
- `ROOM_CREATED`, `ROOM_JOIN`, `ROOM_LEAVE`.
- `UNKNOWN_COMMAND`.

**An toàn thông tin:** Logger không ghi mật khẩu. Các lệnh `REGISTER` và `LOGIN` chỉ được log theo tên lệnh; không ghi tham số mật khẩu.

Ví dụ một số dòng log:

```text
[2026-11-15 10:20:30] SERVER_START port=8080
[2026-11-15 10:20:35] CLIENT_CONNECTED fd=4
[2026-11-15 10:20:40] REGISTER_SUCCESS username=khanh
[2026-11-15 10:20:45] LOGIN_SUCCESS username=khanh fd=4
[2026-11-15 10:20:50] ROOM_CREATED id=1 name=RoomA owner=khanh
[2026-11-15 10:20:55] ROOM_JOIN username=khanh fd=4 room_id=1 members=1
```

Thời gian trong ví dụ chỉ minh họa định dạng; nội dung thực tế phụ thuộc thời điểm chạy.

## 3. Kết quả kiểm thử thực tế

Đã biên dịch và chạy bộ kiểm thử tích hợp trên môi trường hiện tại.

- Biên dịch mặc định: `gcc server_commit5.c -o server_commit5_check -Wall -Wextra` – PASS.
- Biên dịch trong test với `-std=c11 -Wall -Wextra -Wpedantic` – PASS.
- Kiểm thử tích hợp: **37/37 test PASS**.
- Kết quả cuối: `SMOKE_TEST=PASS`.

| Nhóm kiểm thử | Nội dung | Kết quả |
|---|---|---|
| Kết nối TCP | Kết nối client tới server | PASS |
| Giao thức | PING, HELP, nhiều lệnh trong một lần gửi | PASS |
| Tham số | Thiếu tham số, ID không hợp lệ, tên phòng quá dài | PASS |
| Tài khoản | REGISTER, LOGIN và nhiều tài khoản | PASS |
| Quản lý phòng | Tạo phòng, trùng tên, tham gia, rời phòng, liệt kê | PASS |
| Ràng buộc thành viên | Không cho một client tham gia hai phòng | PASS |
| Xác thực | Chặn thao tác phòng khi chưa đăng nhập | PASS |
| Dọn trạng thái | LOGOUT hoặc ngắt kết nối tự động rời phòng | PASS |
| Logger | Ghi sự kiện máy chủ, tài khoản, phòng, lệnh | PASS |
| Bảo mật log | Không lưu mật khẩu trong file log | PASS |

Bộ kiểm thử sử dụng cổng `18080` để tránh xung đột dịch vụ đang dùng cổng `8080`. Mã bàn giao mặc định vẫn dùng cổng `8080`.

## 4. Cách biên dịch và chạy server

```bash
gcc server_commit5.c -o server -Wall -Wextra
./server
```

Kết nối từ terminal khác:

```bash
nc 127.0.0.1 8080
```

Sau khi server ghi nhận sự kiện, xem log bằng:

```bash
cat auction_server.log
```

Theo dõi log liên tục:

```bash
tail -f auction_server.log
```

## 5. Chạy bộ kiểm thử tự động

Yêu cầu: GCC và Python 3.

```bash
python3 test_commit5.py
```

Script tự biên dịch server với cổng kiểm thử `18080`, khởi chạy server, tạo các client TCP, chạy các test và in kết quả PASS/FAIL. File log kiểm thử được đặt trong thư mục tạm và tự dọn sau khi chạy.

## 6. Những giới hạn còn lại

- Tài khoản và phòng vẫn được lưu trong RAM; khởi động lại server sẽ mất dữ liệu.
- Chưa triển khai cơ sở dữ liệu hoặc cơ chế khôi phục dữ liệu.
- Logger mở file theo từng sự kiện; phù hợp quy mô bài tập, chưa phải hệ thống logging tải cao.
- Chưa có chức năng đấu giá thực tế (đặt giá, thời hạn, xác định người thắng).
- Bộ test tự động kiểm tra các luồng chính; chưa thay thế kiểm thử tải hoặc kiểm thử bảo mật chuyên sâu.

## 7. Tiêu chí hoàn thành Commit 5

- [x] Ghi log thời gian khởi động server.
- [x] Ghi log client kết nối và ngắt kết nối.
- [x] Ghi log các thao tác đăng ký, đăng nhập, đăng xuất.
- [x] Ghi log tạo phòng, tham gia phòng và rời phòng.
- [x] Không ghi mật khẩu vào log.
- [x] Biên dịch không có cảnh báo với các cờ kiểm tra đã nêu.
- [x] Chạy bộ kiểm thử tích hợp: 37/37 PASS.
- [x] Có tài liệu hướng dẫn chạy server và test.

## 8. Git commit đề xuất

```bash
git add server_commit5.c test_commit5.py TEST_COMMIT_5.md
git commit -m "feat: add activity logger and integration tests"
```

## 9. Kết luận

Commit 5 đã bổ sung nhật ký hoạt động cho server, tránh ghi mật khẩu vào log, đồng thời cung cấp bộ kiểm thử tích hợp tự động cho các luồng chính từ kết nối TCP, giao thức lệnh, tài khoản đến quản lý phòng. Trong lần chạy kiểm thử tại môi trường hiện tại, cả 37 test đều thành công.
