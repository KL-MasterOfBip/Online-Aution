# Online Auction - C

Ứng dụng đấu giá trực tuyến được xây dựng bằng ngôn ngữ C và TCP Socket.

## Trạng thái hiện tại

Dự án đã hoàn thành phần nền tảng TCP Server, xử lý giao thức lệnh theo dòng (Commit 2), quản lý tài khoản, phiên đăng nhập (Commit 3) quản lý phòng đấu giá (Commit 4) và ghi nhật ký hoạt động cùng bộ kiểm thử tự động (Commit 5).

### Commit 1 – TCP Server nền tảng

- Xây dựng TCP Server bằng C.
- Sử dụng `select()` để phục vụ nhiều client đồng thời.
- Mỗi client có bộ đệm riêng để tiếp nhận và xử lý dữ liệu.
- Server lắng nghe kết nối tại cổng `8080`.

### Commit 2 – Implement TCP Command Protocol

- Sử dụng ký tự xuống dòng `\n` để xác định điểm kết thúc của mỗi lệnh.
- Phân tách và xử lý nhiều lệnh liên tiếp trong cùng luồng TCP.
- Hỗ trợ các lệnh cơ bản:
  - `PING`: kiểm tra kết nối, phản hồi `OK PONG`.
  - `HELP`: hiển thị danh sách lệnh.
  - `QUIT`: kết thúc phiên làm việc, phản hồi `OK Goodbye`.
- Phản hồi khi nhận lệnh không được hỗ trợ.
- Phản hồi khi nhận dòng lệnh rỗng.
- Phát hiện lệnh vượt quá giới hạn bộ đệm và đóng kết nối tương ứng.
- Phát hiện client ngắt kết nối mà không gửi lệnh `QUIT` và giải phóng socket.

### Commit 3 – Account Registration & Login

- Đăng ký tài khoản bằng lệnh `REGISTER`, chỉ dành cho guest (kết nối chưa đăng nhập). Sau `LOGOUT` có thể đăng ký lại.
- Đăng nhập bằng lệnh `LOGIN` và đăng xuất bằng lệnh `LOGOUT`.
- Mỗi kết nối client có một phiên đăng nhập riêng.
- Mỗi client chỉ được đăng nhập một tài khoản tại một thời điểm.
- Mỗi tài khoản chỉ được đăng nhập trên một kết nối tại một thời điểm.
- Tự động giải phóng phiên đăng nhập khi client gửi `QUIT` hoặc ngắt kết nối đột ngột.
- Kiểm tra số lượng tham số và độ dài username, password ở phía server.
- Sai mật khẩu và username không tồn tại dùng chung một thông báo lỗi.
- Server gửi dòng chào `OK Connected to auction server` ngay khi client kết nối.

### Commit 4 – Room Management

- Tạo phòng đấu giá bằng lệnh `CREATE_ROOM`.
- Liệt kê các phòng bằng lệnh `LIST_ROOMS`, gồm ID, tên phòng, chủ phòng và số thành viên.
- Tham gia phòng bằng lệnh `JOIN_ROOM` và rời phòng bằng lệnh `LEAVE_ROOM`.
- Chỉ người dùng đã đăng nhập mới tạo, tham gia hoặc rời phòng.
- Tên phòng không được trùng nhau.
- Mỗi tài khoản chỉ được ở trong một phòng tại một thời điểm.
- Server tự động cập nhật số thành viên khi người dùng rời phòng, đăng xuất, gửi `QUIT` hoặc mất kết nối.
- Kiểm tra ID phòng và số lượng tham số ở phía server.

### Commit 5 – Activity Logger & Basic Tests

- Server ghi các sự kiện quan trọng vào file `auction_server.log` (tạo tại thư mục làm việc khi chạy server), mỗi dòng có thời gian.
- Các sự kiện được ghi: `SERVER_START`, `SERVER_STOP`, `CLIENT_CONNECTED`, `CLIENT_DISCONNECTED`, `COMMAND_RECEIVED`, `REGISTER_SUCCESS`, `LOGIN_SUCCESS`, `LOGOUT`, `ROOM_CREATED`, `ROOM_JOIN`, `ROOM_LEAVE`, `UNKNOWN_COMMAND`.
- `COMMAND_RECEIVED` chỉ ghi tên lệnh, không ghi toàn bộ nội dung lệnh.
- Logger **không ghi mật khẩu**: `REGISTER` và `LOGIN` chỉ được ghi theo tên lệnh.
- Bổ sung bộ kiểm thử tự động `test_commit5.py` (chỉ dùng thư viện chuẩn Python) cho các luồng chính: kết nối TCP, giao thức lệnh, tài khoản, phòng và logger.

Ví dụ nội dung file log (thời gian chỉ minh họa định dạng):

```text
[2026-11-15 10:20:30] SERVER_START port=8080
[2026-11-15 10:20:35] CLIENT_CONNECTED fd=4
[2026-11-15 10:20:40] REGISTER_SUCCESS username=khanh
[2026-11-15 10:20:45] LOGIN_SUCCESS username=khanh fd=4
[2026-11-15 10:20:50] ROOM_CREATED id=1 name=RoomA owner=khanh
[2026-11-15 10:20:55] ROOM_JOIN username=khanh fd=4 room_id=1 members=1
```

### Các lệnh hiện có

| Lệnh | Chức năng | Phản hồi thành công |
| --- | --- | --- |
| `PING` | Kiểm tra kết nối | `OK PONG` |
| `HELP` | Danh sách lệnh | `OK Commands: ...` (danh sách các lệnh đang hỗ trợ) |
| `REGISTER <username> <password>` | Đăng ký tài khoản | `OK Account registered` |
| `LOGIN <username> <password>` | Đăng nhập | `OK Login successful` |
| `LOGOUT` | Đăng xuất (tự động rời phòng nếu đang ở trong phòng) | `OK Logout successful` |
| `CREATE_ROOM <ten_phong>` | Tạo phòng đấu giá | `OK ROOM_CREATED <id>` |
| `LIST_ROOMS` | Liệt kê các phòng | Danh sách phòng, kết thúc bằng `END_ROOM_LIST` |
| `JOIN_ROOM <room_id>` | Tham gia phòng | `OK JOINED_ROOM <id>` |
| `LEAVE_ROOM` | Rời phòng hiện tại | `OK LEFT_ROOM` |
| `QUIT` | Kết thúc phiên làm việc | `OK Goodbye` |

### Các phản hồi lỗi

| Tình huống | Phản hồi |
| --- | --- |
| Lệnh không được hỗ trợ | `ERROR Unknown command: <lệnh>` |
| Dòng lệnh rỗng | `ERROR Empty command` |
| Lệnh dài hơn giới hạn bộ đệm | `ERROR Command too long` (đóng kết nối) |
| Sai số lượng tham số | `ERROR Invalid command or arguments` |
| Username đã tồn tại | `ERROR Username already exists` |
| Username hoặc password quá dài | `ERROR Username or password is too long` |
| Sai username hoặc password | `ERROR Invalid username or password` |
| Client đã đăng nhập mà gửi `LOGIN` hoặc `REGISTER` | `ERROR Already logged in` |
| Tài khoản đang đăng nhập ở kết nối khác | `ERROR Account already logged in` |
| `LOGOUT`, `CREATE_ROOM`, `JOIN_ROOM`, `LEAVE_ROOM` khi chưa đăng nhập | `ERROR Not logged in` |
| Tên phòng đã tồn tại | `ERROR Room already exists` |
| Tham gia phòng khi đang ở phòng khác | `ERROR Already in a room` |
| Tham gia phòng không tồn tại | `ERROR Room not found` |
| `LEAVE_ROOM` khi chưa ở trong phòng | `ERROR Not in a room` |
| ID phòng không hợp lệ (`abc`, `-1`) | `ERROR Invalid room ID` |
| Hết chỗ lưu tài khoản | `ERROR User database is full` |
| Hết chỗ cho client mới | `ERROR Server is full` |

### Giới hạn dữ liệu

Trong mã nguồn:

```c
#define BUFFER_SIZE 1024
#define MAX_USERS 100
#define USERNAME_SIZE 32
#define PASSWORD_SIZE 64
```

Do cần dành một byte cho ký tự kết thúc chuỗi `\0`:

- Nội dung một dòng lệnh tối đa **1023 byte**.
- Username tối đa **31 ký tự**.
- Password tối đa **63 ký tự**.
- Tối đa **100 tài khoản** trong một lần chạy server.

> **Lưu ý về TCP:** Một lần `send()` hoặc `sendall()` từ client không đảm bảo dữ liệu được nhận trong đúng một lần `recv()` ở server. TCP là luồng byte và không bảo toàn ranh giới giữa các lần gửi. Vì vậy, server phải tự phân tách lệnh dựa trên ký tự `\n`.

> **Lưu ý về lưu trữ:** Tài khoản và phòng hiện được lưu trong RAM. Dữ liệu vẫn còn khi client ngắt kết nối nhưng sẽ mất khi tắt hoặc khởi động lại server. Mật khẩu chưa được băm. Lưu trữ bền vững sẽ được bổ sung ở giai đoạn sau.

## Biên dịch

Nếu sử dụng Makefile:

```bash
make
```

Hoặc biên dịch trực tiếp:

```bash
gcc -Wall -Wextra -std=c11 -g src/server.c -o server
```

## Chạy server

```bash
./server
```

Server lắng nghe tại cổng `8080`.

## Kiểm tra server

Mở Terminal 1 và chạy server:

```bash
./server
```

Mở Terminal 2, kết nối bằng netcat:

```bash
nc 127.0.0.1 8080
```

Thử lần lượt các lệnh:

```text
PING
HELP
REGISTER khanh 123456
LOGIN khanh 123456
CREATE_ROOM Phong1
LIST_ROOMS
JOIN_ROOM 1
LEAVE_ROOM
LOGOUT
QUIT
```

Có thể mở nhiều terminal để tạo nhiều kết nối client đồng thời. Ví dụ, mở Terminal 3 và thử `LOGIN khanh 123456` trong khi Terminal 2 đang đăng nhập cùng tài khoản; server phải từ chối. Với phòng, thử `JOIN_ROOM 2` khi đang ở phòng 1; server phải trả về `ERROR Already in a room`.

## Xem log

Sau khi server chạy và có client thao tác, xem log bằng:

```bash
cat auction_server.log
```

Theo dõi log liên tục:

```bash
tail -f auction_server.log
```

## Chạy bộ kiểm thử tự động

Yêu cầu: GCC và Python 3.

```bash
python3 test_commit5.py
```

Script tự biên dịch server với cổng kiểm thử `18080` (để tránh xung đột với cổng `8080`), khởi chạy server, tạo các client TCP, chạy các test và in kết quả PASS/FAIL. File log của lần kiểm thử được đặt trong thư mục tạm và tự dọn sau khi chạy. Mã mặc định vẫn dùng cổng `8080`.

## Các trường hợp kiểm thử Commit 2

| Test | Nội dung | Kết quả |
| --- | --- | --- |
| 1 | Kết nối và gửi `PING` | Server trả về `OK PONG` |
| 2 | Gửi lệnh không hợp lệ (`ABC`) | Server trả về lỗi `Unknown command` |
| 3 | Gửi một dòng rỗng | Server trả về `ERROR Empty command` |
| 4 | Gửi `PING`, `HELP`, `QUIT` liên tiếp trong một lần gửi | Server xử lý đủ cả 3 lệnh theo thứ tự |
| 5 | Gửi lệnh dài hơn giới hạn bộ đệm | Server báo `ERROR Command too long` và đóng kết nối |
| 6 | Client đóng kết nối mà không gửi `QUIT` | Server phát hiện client ngắt kết nối và đóng socket |

**Trạng thái kiểm thử:** Đã chạy thực tế và xác nhận cả 6 bài kiểm thử đều thành công.

## Các trường hợp kiểm thử Commit 3

| Test | Nội dung | Kết quả |
| --- | --- | --- |
| 1 | Đăng ký tài khoản và đăng ký trùng | `OK Account registered` / `ERROR Username already exists` |
| 2 | Đăng ký sai cú pháp (thiếu hoặc thừa tham số) | `ERROR Invalid command or arguments` |
| 3 | Đăng nhập đúng và đăng nhập sai | `OK Login successful` / `ERROR Invalid username or password` |
| 4 | Một client đăng nhập hai tài khoản | `ERROR Already logged in` |
| 5 | Một tài khoản đăng nhập từ hai client | `ERROR Account already logged in` |
| 6 | Đăng xuất | `OK Logout successful` / `ERROR Not logged in` |
| 7 | Client ngắt kết nối đột ngột khi đang đăng nhập | Tài khoản được giải phóng, đăng nhập lại được |
| 8 | Gửi `QUIT` khi đang đăng nhập | `OK Goodbye`, đóng kết nối, giải phóng tài khoản |
| 9 | Username hoặc password quá dài | `ERROR Username or password is too long` |
| 10 | `PING`, `HELP` sau khi bổ sung tài khoản | Vẫn hoạt động bình thường |

Chi tiết từng bước nằm trong `TONG_KET_COMMIT_3.md`.

**Trạng thái kiểm thử:** Chưa thực hiện. Chỉ cập nhật thành "Đã chạy thực tế và xác nhận thành công" sau khi chạy xong các test trên.

## Các trường hợp kiểm thử Commit 4

| Test | Nội dung | Kết quả mong đợi | Trạng thái |
| --- | --- | --- | --- |
| TC01 | Tạo phòng hợp lệ | `OK ROOM_CREATED <id>` | Đạt (smoke test) |
| TC02 | Tạo phòng trùng tên | `ERROR Room already exists` | Chưa chạy |
| TC03 | Liệt kê phòng | Danh sách phòng và `END_ROOM_LIST` | Đạt (smoke test) |
| TC04 | Tham gia phòng hợp lệ | `OK JOINED_ROOM <id>`, số thành viên tăng 1 | Đạt (smoke test) |
| TC05 | Tham gia phòng thứ hai khi đang ở phòng 1 | `ERROR Already in a room` | Đạt (smoke test) |
| TC06 | Rời phòng | `OK LEFT_ROOM`, số thành viên giảm 1 | Đạt (smoke test) |
| TC07 | Rời phòng 1 rồi tham gia phòng 2 | Tham gia thành công | Chưa chạy |
| TC08 | Tạo phòng khi chưa đăng nhập | `ERROR Not logged in` | Chưa chạy |
| TC09 | Tham gia phòng không tồn tại | `ERROR Room not found` | Chưa chạy |
| TC10 | Mất kết nối khi đang ở phòng | Số thành viên tự giảm | Chưa chạy |
| TC11 | Đăng xuất khi đang ở phòng | Đăng xuất thành công, số thành viên giảm 1 | Đạt một phần (mới thử `LOGOUT` sau `LEAVE_ROOM`) |
| TC12 | Tham gia phòng khi chưa đăng nhập | `ERROR Not logged in` | Chưa chạy |
| TC13 | Rời phòng khi chưa ở phòng nào | `ERROR Not in a room` | Chưa chạy |
| TC14 | ID phòng không hợp lệ | `ERROR Invalid room ID` | Chưa chạy |
| TC15 | Sai số lượng tham số | `ERROR Invalid command or arguments` | Chưa chạy |

Chi tiết từng bước nằm trong `TEST_COMMIT_4.md`.

**Trạng thái kiểm thử:** Mới chạy smoke test cho chuỗi `REGISTER` → `LOGIN` → `CREATE_ROOM` → `LIST_ROOMS` → `JOIN_ROOM` → `JOIN_ROOM` (bị từ chối) → `LEAVE_ROOM` → `LOGOUT` → `QUIT`. Các test còn lại chưa chạy; chỉ chuyển sang "Đạt" sau khi thực thi và quan sát đúng kết quả.

## Các trường hợp kiểm thử Commit 5

| Nhóm kiểm thử | Nội dung | Kết quả |
| --- | --- | --- |
| Kết nối TCP | Kết nối client tới server | PASS |
| Giao thức | `PING`, `HELP`, nhiều lệnh trong một lần gửi | PASS |
| Tham số | Thiếu tham số, ID không hợp lệ, tên phòng quá dài | PASS |
| Tài khoản | `REGISTER`, `LOGIN` và nhiều tài khoản | PASS |
| Quản lý phòng | Tạo phòng, trùng tên, tham gia, rời phòng, liệt kê | PASS |
| Ràng buộc thành viên | Không cho một client tham gia hai phòng | PASS |
| Xác thực | Chặn thao tác phòng khi chưa đăng nhập | PASS |
| Dọn trạng thái | `LOGOUT` hoặc ngắt kết nối tự động rời phòng | PASS |
| Logger | Ghi sự kiện máy chủ, tài khoản, phòng, lệnh | PASS |
| Bảo mật log | Không lưu mật khẩu trong file log | PASS |

Chi tiết nằm trong `TEST_COMMIT_5.md`.

**Trạng thái kiểm thử:** Theo `TEST_COMMIT_5.md`, bộ kiểm thử tích hợp đã chạy với kết quả **37/37 test PASS**, biên dịch không có cảnh báo với `-Wall -Wextra` (và `-std=c11 -Wall -Wextra -Wpedantic` trong script test). Bộ test tự động chỉ kiểm tra các luồng chính, chưa thay thế kiểm thử tải hoặc kiểm thử bảo mật chuyên sâu.

## Quy ước giao thức

- Client gửi mỗi lệnh trên một dòng.
- Mỗi lệnh kết thúc bằng ký tự xuống dòng `\n`.
- Các thành phần của lệnh phân cách bằng khoảng trắng, nên username và password không được chứa khoảng trắng.
- Server phân tách dữ liệu nhận được thành từng lệnh trước khi xử lý.
- Server gửi phản hồi cho từng lệnh, bắt đầu bằng `OK` hoặc `ERROR`.
- Server kiểm tra lệnh, số tham số và độ dài dữ liệu trước khi xử lý.
- Trạng thái đăng nhập do server quản lý, không dựa vào dữ liệu client gửi lên.
- Khi client ngắt kết nối, server đóng socket, giải phóng phiên đăng nhập, rút khỏi phòng đang tham gia và giải phóng tài nguyên tương ứng.

## Hạn chế hiện tại

- Tài khoản và phòng chỉ lưu trong RAM, mất khi khởi động lại server.
- Mật khẩu chưa được băm.
- Chưa kiểm tra ký tự hợp lệ của username.
- `QUIT` còn hai trường hợp biên: `QUIT extra` trả về lỗi nhưng vẫn đóng kết nối, còn `  QUIT` (có khoảng trắng đầu dòng) trả về `OK Goodbye` nhưng không đóng kết nối.
- Chưa có lệnh xóa phòng.
- Chưa giới hạn số người trong một phòng.
- Người tạo phòng chưa tự động trở thành thành viên của phòng.
- Chưa có vật phẩm, đặt giá và mua ngay.
- Chưa có unit test đo độ phủ (coverage) và chưa có kiểm thử tải; bộ test hiện tại là kiểm thử tích hợp.
- Logger mở file theo từng sự kiện, phù hợp quy mô bài tập, chưa phải hệ thống logging tải cao.
- Server xử lý một luồng trong vòng lặp `select()`; nếu chuyển sang đa luồng cần đồng bộ dữ liệu dùng chung.

## Các chức năng dự kiến phát triển tiếp

Các chức năng nghiệp vụ của ứng dụng đấu giá sẽ được triển khai trong những commit tiếp theo, bao gồm:

- Quản lý vật phẩm đấu giá (tạo, liệt kê, rút vật phẩm chưa mở phiên).
- Đặt giá và cập nhật trạng thái phiên đấu giá.
- Mua ngay và bộ đếm 3 phút kèm thông báo.
- Lưu trữ dữ liệu bền vững, unit test đạt coverage trên 90% và kiểm thử tải.
