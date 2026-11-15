# Tổng kết kiểm thử Commit 2 – Implement Command Protocol

Mục tiêu của Commit 2 là hoàn thiện và kiểm tra khả năng xử lý giao thức TCP theo dòng trong chương trình Auction Server viết bằng C.

Server sử dụng ký tự xuống dòng `\n` để xác định điểm kết thúc mỗi lệnh.

## 1. Test 1 – Kiểm tra kết nối và lệnh PING

**Cách thực hiện:**

Terminal 1:

```bash
./server
```

Terminal 2:

```bash
nc 127.0.0.1 8080
```

Nhập:

```text
PING
```

Kết quả mong đợi:

```text
OK PONG
```

**Mục đích:** Kiểm tra client kết nối được với server và server xử lý lệnh PING.

## 2. Test 2 – Kiểm tra lệnh không hợp lệ

Kết nối bằng `nc`:

```bash
nc 127.0.0.1 8080
```

Nhập:

```text
ABC
```

Kết quả mong đợi:

```text
ERROR Unknown command: ABC
```

**Mục đích:** Kiểm tra server nhận diện và phản hồi khi client gửi lệnh không được hỗ trợ.

## 3. Test 3 – Kiểm tra dòng lệnh rỗng

Kết nối bằng `nc`:

```bash
nc 127.0.0.1 8080
```

Không nhập nội dung, chỉ nhấn Enter.

Kết quả mong đợi:

```text
ERROR Empty command
```

**Mục đích:** Kiểm tra server xử lý trường hợp client gửi một dòng không có nội dung.

## 4. Test 4 – Kiểm tra nhiều lệnh trong một lần gửi

**Cách thực hiện:**

Giữ server chạy ở Terminal 1. Tại Terminal 2, chạy:

```bash
python3 - <<'PY'
import socket

client = socket.create_connection(("127.0.0.1", 8080))

print(client.recv(1024).decode())

# Gửi 3 lệnh liên tiếp trong một lần gửi
client.sendall(b"PING\nHELP\nQUIT\n")

while True:
    data = client.recv(1024)

    if not data:
        break

    print(data.decode(), end="")

client.close()
PY
```

Kết quả mong đợi:

```text
OK Connected to auction server
OK PONG
OK Commands: HELP, PING, QUIT
OK Goodbye
```

**Mục đích:** Kiểm tra server có thể phân tách và xử lý nhiều lệnh liên tiếp trong cùng luồng TCP.

> **Lưu ý:** Một lần `sendall()` không đảm bảo dữ liệu được nhận trong đúng một lần `recv()`. TCP không bảo toàn ranh giới giữa các lần gửi.

## 5. Test 5 – Kiểm tra lệnh quá dài

Trong code:

```c
#define BUFFER_SIZE 1024
```

Giới hạn nội dung một dòng là 1023 byte, vì cần dành một byte cho ký tự `\0`.

**Cách thực hiện:**

Tại Terminal 2, chạy:

```bash
python3 - <<'PY'
import socket

client = socket.create_connection(("127.0.0.1", 8080))

print(client.recv(1024).decode())

# Gửi 1100 ký tự A
client.sendall(b"A" * 1100 + b"\n")

while True:
    try:
        data = client.recv(1024)

        if not data:
            break

        print(data.decode(), end="")

    except ConnectionResetError:
        break

client.close()
PY
```

Kết quả mong đợi:

```text
OK Connected to auction server
ERROR Command too long
```

**Mục đích:** Kiểm tra server phát hiện lệnh vượt giới hạn bộ đệm, gửi thông báo lỗi và đóng kết nối client.

## 6. Test 6 – Kiểm tra client ngắt kết nối bất thường

**Cách thực hiện:**

Tại Terminal 2, chạy:

```bash
python3 - <<'PY'
import socket

client = socket.create_connection(("127.0.0.1", 8080))

print(client.recv(1024).decode())

# Đóng kết nối trực tiếp, không gửi QUIT
client.close()

print("Client da ngat ket noi.")
PY
```

Kết quả mong đợi ở Terminal 2:

```text
OK Connected to auction server

Client da ngat ket noi.
```

Kết quả mong đợi ở Terminal 1:

```text
Client connected: 4
Client disconnected: 4
```

Số socket có thể khác nhau tùy máy.

**Mục đích:** Kiểm tra server phát hiện client đóng kết nối mà không gửi lệnh QUIT.

Server thực hiện việc này thông qua:

```c
if (received <= 0) {
    printf("Client disconnected: %d\n", client_fd);
    close_client(i);
    continue;
}
```

## 7. Tổng hợp 6 bài kiểm thử

| Test | Nội dung             | Kết quả cần đạt                 |
| ---- | -------------------- | ------------------------------- |
| 1    | Kết nối và PING      | Trả về `OK PONG`                |
| 2    | Lệnh không hợp lệ    | Trả về `ERROR Unknown command`  |
| 3    | Dòng rỗng            | Trả về `ERROR Empty command`    |
| 4    | Nhiều lệnh liên tiếp | Xử lý đủ PING, HELP, QUIT       |
| 5    | Lệnh quá dài         | Báo lỗi và đóng kết nối         |
| 6    | Client ngắt kết nối  | Server phát hiện và đóng socket |

## 8. Sau khi kiểm thử thành công

Biên dịch lại:

```bash
gcc -Wall -Wextra -std=c11 server.c -o server
```

Sau đó đưa mã nguồn và tài liệu kiểm thử vào Git:

```bash
git add .
git commit -m "Implement TCP command protocol"
git push
```

### Mục tiêu Commit 2

Hoàn thiện khả năng tiếp nhận, phân tách, xử lý lệnh và quản lý kết nối TCP của client.

> **Lưu ý:** Hiện tại đây là kế hoạch kiểm thử và kết quả mong đợi, chưa phải xác nhận 6 test đã chạy thành công. Chỉ đánh dấu Commit 2 hoàn thành sau khi bạn thực sự chạy và kiểm tra từng test.
