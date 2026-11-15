# TEST_COMMIT_4 – Kiểm thử chức năng quản lý phòng đấu giá

## 1. Thông tin

- Dự án: Ứng dụng đấu giá trực tuyến bằng ngôn ngữ C.
- Phạm vi: Commit 4 – Quản lý phòng đấu giá.
- Server: TCP socket, xử lý nhiều client bằng `select()`.
- Lưu trữ: Dữ liệu tài khoản và phòng được lưu trong RAM.

## 2. Các chức năng được bổ sung

| Lệnh | Chức năng | Điều kiện chính |
|---|---|---|
| `CREATE_ROOM <ten_phong>` | Tạo phòng | Phải đăng nhập; tên phòng không trùng; còn chỗ lưu |
| `LIST_ROOMS` | Liệt kê phòng | Trả về ID, tên, chủ phòng và số thành viên |
| `JOIN_ROOM <room_id>` | Tham gia phòng | Phải đăng nhập; phòng tồn tại; client chưa ở phòng khác |
| `LEAVE_ROOM` | Rời phòng | Phải đăng nhập và đang ở trong một phòng |

Các trường hợp tự động rời phòng:
- Người dùng gửi `LEAVE_ROOM`.
- Người dùng gửi `LOGOUT` khi đang ở trong phòng.
- Client mất kết nối hoặc gửi `QUIT` khi đang ở trong phòng.

## 3. Danh sách test case

**Lưu ý về trạng thái:** Các test case dưới đây là kế hoạch kiểm thử và tiêu chí mong đợi. Chúng chưa được xác nhận là đã chạy thực tế trên máy của người dùng. Đánh dấu kết quả sau khi chạy chương trình.

| ID | Tình huống kiểm thử | Thao tác | Kết quả mong đợi | Kết quả thực tế |
|---|---|---|---|---|
| TC01 | Tạo phòng hợp lệ | Đăng nhập, gửi `CREATE_ROOM Phong1` | `OK ROOM_CREATED <id>` | Đạt (smoke test) |
| TC02 | Tạo phòng trùng tên | Gửi lại `CREATE_ROOM Phong1` | `ERROR Room already exists` | Chưa chạy |
| TC03 | Liệt kê phòng | Gửi `LIST_ROOMS` | Danh sách có thông tin phòng và `END_ROOM_LIST` | Đạt (smoke test) |
| TC04 | Tham gia phòng hợp lệ | Gửi `JOIN_ROOM 1` với phòng tồn tại | `OK JOINED_ROOM 1`; số thành viên tăng 1 | Đạt (smoke test) |
| TC05 | Tham gia phòng thứ hai | Đang ở phòng 1, gửi `JOIN_ROOM 2` | `ERROR Already in a room`; số thành viên không đổi | Đạt (smoke test) |
| TC06 | Rời phòng | Đang ở phòng, gửi `LEAVE_ROOM` | `OK LEFT_ROOM`; số thành viên giảm 1 | Đạt (smoke test) |
| TC07 | Tham gia phòng khác sau khi rời | Rời phòng 1 rồi gửi `JOIN_ROOM 2` | Tham gia phòng 2 thành công | Chưa chạy |
| TC08 | Tạo phòng khi chưa đăng nhập | Chưa đăng nhập, gửi `CREATE_ROOM Phong3` | `ERROR Not logged in` | Chưa chạy |
| TC09 | Tham gia phòng không tồn tại | Đăng nhập, gửi `JOIN_ROOM 9999` | `ERROR Room not found` | Chưa chạy |
| TC10 | Mất kết nối khi đang ở phòng | Tham gia phòng rồi đóng client đột ngột | Server tự giảm số thành viên của phòng | Chưa chạy |
| TC11 | Đăng xuất khi đang ở phòng | Đang ở phòng, gửi `LOGOUT` | Đăng xuất thành công và số thành viên giảm 1 | Đạt (smoke test: LOGOUT sau LEAVE_ROOM; cần test thêm khi vẫn đang ở phòng) |
| TC12 | Tham gia phòng khi chưa đăng nhập | Chưa đăng nhập, gửi `JOIN_ROOM 1` | `ERROR Not logged in` | Chưa chạy |
| TC13 | Rời phòng khi chưa ở phòng nào | Đăng nhập nhưng chưa tham gia phòng, gửi `LEAVE_ROOM` | `ERROR Not in a room` | Chưa chạy |
| TC14 | ID phòng không hợp lệ | Gửi `JOIN_ROOM abc` hoặc `JOIN_ROOM -1` | `ERROR Invalid room ID` | Chưa chạy |
| TC15 | Sai số lượng tham số | Gửi `CREATE_ROOM`, `LIST_ROOMS extra` hoặc `LEAVE_ROOM extra` | `ERROR Invalid command or arguments` | Chưa chạy |

### Kết quả smoke test đã thực hiện

Đã chạy thành công chuỗi lệnh: `REGISTER` → `LOGIN` → `CREATE_ROOM` → `LIST_ROOMS` → `JOIN_ROOM` → thử tham gia phòng thứ hai (bị từ chối đúng) → `LEAVE_ROOM` → `LOGOUT` → `QUIT`. Kết quả: `SMOKE_TEST=PASS`. Bản kiểm thử dùng cổng 18080 để tránh xung đột với dịch vụ đang sử dụng cổng 8080; mã bàn giao vẫn cấu hình cổng 8080.

## 4. Cách biên dịch và chạy

```bash
gcc server_commit4.c -o server -Wall -Wextra
./server
```

Mở terminal khác để kết nối thử bằng Netcat:

```bash
nc 127.0.0.1 8080
```

Có thể mở hai terminal Netcat để kiểm tra nhiều client cùng tham gia một phòng.

## 5. Kịch bản kiểm thử mẫu

Tại client thứ nhất:

```text
REGISTER khanh 123456
LOGIN khanh 123456
CREATE_ROOM Phong1
LIST_ROOMS
JOIN_ROOM 1
```

Tại client thứ hai, đăng ký và đăng nhập bằng tài khoản khác, sau đó:

```text
JOIN_ROOM 1
LIST_ROOMS
```

Quay lại client thứ nhất và thử:

```text
CREATE_ROOM Phong2
JOIN_ROOM 2
```

Lệnh `JOIN_ROOM 2` phải bị từ chối vì client thứ nhất đang ở phòng 1. Sau đó:

```text
LEAVE_ROOM
JOIN_ROOM 2
```

Lần tham gia phòng 2 phải thành công.

## 6. Tiêu chí hoàn thành Commit 4

- [x] Tạo được phòng với tên hợp lệ (smoke test).
- [ ] Không tạo được phòng trùng tên.
- [x] Liệt kê đúng các phòng đang hoạt động (smoke test).
- [x] Client đã đăng nhập tham gia được phòng tồn tại (smoke test).
- [x] Một client không thể tham gia hai phòng cùng lúc (smoke test).
- [x] Lệnh rời phòng cập nhật trạng thái (smoke test; cần đối chiếu số thành viên bằng LIST_ROOMS).
- [ ] Đăng xuất tự động rời phòng.
- [ ] Mất kết nối tự động rời phòng.
- [ ] Các lệnh sai tham số được xử lý phù hợp.
- [ ] Biên dịch không có cảnh báo với `-Wall -Wextra`.

## 7. Giới hạn hiện tại

- Chưa có lệnh xóa phòng.
- Chưa giới hạn số người trong một phòng.
- Người tạo phòng chưa tự động trở thành thành viên.
- Chưa triển khai đặt giá, đấu giá hoặc xác định người thắng.
- Dữ liệu chưa được lưu xuống file hoặc cơ sở dữ liệu; khởi động lại server sẽ mất dữ liệu.
- Server đang xử lý trạng thái phòng trong một vòng lặp `select()` đơn luồng; nếu chuyển sang đa luồng cần đồng bộ dữ liệu dùng chung.

## 8. Kết luận

Mã nguồn Commit 4 đã được bổ sung các lệnh quản lý phòng và cơ chế cập nhật thành viên khi tham gia, rời phòng, đăng xuất hoặc mất kết nối. Tuy nhiên, các test case trong tài liệu hiện là tiêu chí kiểm thử dự kiến, chưa phải biên bản xác nhận chạy thành công. Chỉ chuyển trạng thái từng test sang **Đạt** sau khi thực thi và quan sát đúng kết quả mong đợi.
