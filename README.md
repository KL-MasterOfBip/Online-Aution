# Online Auction - C

Ứng dụng đấu giá trực tuyến được xây dựng bằng ngôn ngữ C và TCP Socket.

## Trạng thái hiện tại

Dự án đã hoàn thành phần nền tảng TCP Server và xử lý giao thức lệnh theo dòng (Commit 2).

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
  - `HELP`: hiển thị danh sách lệnh, phản hồi `OK Commands: HELP, PING, QUIT`.
  - `QUIT`: kết thúc phiên làm việc, phản hồi `OK Goodbye`.
- Phản hồi khi nhận lệnh không được hỗ trợ.
- Phản hồi khi nhận dòng lệnh rỗng.
- Phát hiện lệnh vượt quá giới hạn bộ đệm và đóng kết nối tương ứng.
- Phát hiện client ngắt kết nối mà không gửi lệnh `QUIT` và giải phóng socket.

### Giới hạn dữ liệu

Trong mã nguồn, bộ đệm được khai báo:

```c
#define BUFFER_SIZE 1024
```

Do cần dành một byte cho ký tự kết thúc chuỗi `\0`, độ dài tối đa của nội dung một dòng lệnh là **1023 byte**.

> **Lưu ý về TCP:** Một lần `send()` hoặc `sendall()` từ client không đảm bảo dữ liệu được nhận trong đúng một lần `recv()` ở server. TCP là luồng byte và không bảo toàn ranh giới giữa các lần gửi. Vì vậy, server phải tự phân tách lệnh dựa trên ký tự `\n`.

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
QUIT
```

Có thể mở nhiều terminal để tạo nhiều kết nối client đồng thời.

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

## Quy ước giao thức

- Client gửi mỗi lệnh trên một dòng.
- Mỗi lệnh kết thúc bằng ký tự xuống dòng `\n`.
- Server phân tách dữ liệu nhận được thành từng lệnh trước khi xử lý.
- Server gửi phản hồi cho từng lệnh.
- Server kiểm tra lệnh và dữ liệu đầu vào trước khi xử lý.
- Khi client ngắt kết nối, server đóng socket và giải phóng tài nguyên tương ứng.

## Các chức năng dự kiến phát triển tiếp

Các chức năng nghiệp vụ của ứng dụng đấu giá sẽ được triển khai trong những commit tiếp theo, bao gồm:

- Tạo và quản lý tài khoản người dùng.
- Tạo phòng đấu giá và tham gia phòng đấu giá.
- Quản lý vật phẩm đấu giá.
- Đặt giá và cập nhật trạng thái phiên đấu giá.