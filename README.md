# Online Auction - C

Ứng dụng đấu giá trực tuyến được xây dựng bằng C và TCP Socket.

## Trạng thái hiện tại

Đây là phiên bản nền tảng đầu tiên:

- TCP server sử dụng `select()` để phục vụ nhiều client.
- Mỗi client có bộ đệm riêng để xử lý dữ liệu TCP theo dòng.
- Giao thức trao đổi dùng ký tự xuống dòng `\n` để kết thúc một lệnh.
- Có các lệnh thử nghiệm `HELP`, `PING`, `QUIT`.

Các chức năng tài khoản, phòng đấu giá, vật phẩm và đặt giá sẽ được phát triển ở những commit tiếp theo.

## Biên dịch

```bash
make
```

Hoặc:

```bash
gcc -Wall -Wextra -std=c11 -g src/server.c -o server
```

## Chạy server

```bash
./server
```

Server lắng nghe tại cổng `8080`.

## Kiểm tra bằng netcat

Mở terminal thứ nhất:

```bash
./server
```

Mở terminal thứ hai:

```bash
nc 127.0.0.1 8080
```

Nhập lần lượt:

```text
PING
HELP
QUIT
```

Có thể mở nhiều terminal và kết nối nhiều client cùng lúc.

## Quy ước giao thức

- Client gửi một lệnh trên một dòng.
- Mỗi lệnh kết thúc bằng `\\n`.
- Server trả về một dòng phản hồi.
- Server là nơi quản lý trạng thái và kiểm tra tính hợp lệ của các thao tác.
