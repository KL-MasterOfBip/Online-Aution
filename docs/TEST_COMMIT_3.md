# Tổng kết kiểm thử Commit 3 – Account Registration & Login

Mục tiêu của Commit 3 là bổ sung quản lý tài khoản vào Auction Server viết bằng C: đăng ký, đăng nhập, đăng xuất và quản lý phiên đăng nhập của từng client.

Giao thức vẫn dùng ký tự xuống dòng `\n` để xác định điểm kết thúc mỗi lệnh. Tài khoản hiện được lưu trong RAM nên sẽ mất khi khởi động lại server.

Các lệnh mới:

| Lệnh | Chức năng |
| ---- | --------- |
| `REGISTER <username> <password>` | Đăng ký tài khoản |
| `LOGIN <username> <password>` | Đăng nhập |
| `LOGOUT` | Đăng xuất |

## Chuẩn bị

Biên dịch và chạy server:

```bash
gcc -Wall -Wextra -std=c11 server.c -o server
./server
```

> **Lưu ý:** Tài khoản nằm trong RAM, nên các test dưới đây được viết theo thứ tự trên **một lần chạy server mới**. Nếu chạy lại, hãy tắt và bật lại server (hoặc đổi username).

Khi kết nối, server luôn gửi dòng chào:

```text
OK Connected to auction server
```

## 1. Test 1 – Đăng ký tài khoản và đăng ký trùng

Terminal 2:

```bash
nc 127.0.0.1 8080
```

Nhập:

```text
REGISTER khanh 123456
REGISTER khanh abcdef
```

Kết quả mong đợi:

```text
OK Account registered
ERROR Username already exists
```

**Mục đích:** Kiểm tra server tạo được tài khoản mới và không cho đăng ký trùng username.

## 2. Test 2 – Đăng ký sai cú pháp

Nhập:

```text
REGISTER khanh
REGISTER user3 333333 extra
```

Kết quả mong đợi (cho từng lệnh):

```text
ERROR Invalid command or arguments
```

**Mục đích:** Kiểm tra server từ chối lệnh thiếu hoặc thừa tham số, không bỏ qua âm thầm và không bị crash.

## 3. Test 3 – Đăng nhập đúng và đăng nhập sai

Nhập:

```text
LOGIN khanh sai_mat_khau
LOGIN nguoidung_khong_co 123456
LOGIN khanh 123456
```

Kết quả mong đợi:

```text
ERROR Invalid username or password
ERROR Invalid username or password
OK Login successful
```

**Mục đích:** Kiểm tra server chỉ cho đăng nhập khi đúng username và password. Sai mật khẩu và username không tồn tại dùng chung một thông báo.

## 4. Test 4 – Một client không đăng nhập hai tài khoản

Giữ client đang đăng nhập `khanh` ở Test 3. Tại cùng client này nhập:

```text
REGISTER user2 222222
LOGIN user2 222222
```

> Cả `REGISTER` và `LOGIN` đều bị từ chối với `ERROR Already logged in`. Chỉ guest mới được đăng ký. Sau `LOGOUT`, thử `REGISTER user2 222222`: phải nhận `OK Account registered`, xác nhận lần đăng ký bị chặn trước đó không tạo tài khoản.

Kết quả mong đợi của lệnh `LOGIN user2 222222`:

```text
ERROR Already logged in
```

**Mục đích:** Kiểm tra mỗi kết nối chỉ gắn với tối đa một tài khoản.

## 5. Test 5 – Một tài khoản không đăng nhập từ hai client

Giữ client 1 đang đăng nhập `khanh`. Mở Terminal 3:

```bash
nc 127.0.0.1 8080
```

Nhập:

```text
LOGIN khanh 123456
```

Kết quả mong đợi tại client 2:

```text
ERROR Account already logged in
```

**Mục đích:** Kiểm tra trạng thái đăng nhập được dùng chung giữa các kết nối.

## 6. Test 6 – Đăng xuất

Tại client 1 nhập:

```text
LOGOUT
LOGOUT
```

Kết quả mong đợi:

```text
OK Logout successful
ERROR Not logged in
```

Sau đó tại client 2 nhập:

```text
LOGIN khanh 123456
```

Kết quả mong đợi:

```text
OK Login successful
```

**Mục đích:** Kiểm tra `LOGOUT` giải phóng tài khoản, và đăng xuất khi chưa đăng nhập bị từ chối.

## 7. Test 7 – Client ngắt kết nối đột ngột khi đang đăng nhập

**Cách thực hiện:**

Giữ server chạy ở Terminal 1. Tại Terminal 2, chạy:

```bash
python3 - <<'PY'
import socket

def connect():
    client = socket.create_connection(("127.0.0.1", 8080))
    client.recv(1024)  # bỏ dòng chào
    return client

def send(client, line):
    client.sendall((line + "\n").encode())
    return client.recv(1024).decode().strip()

a = connect()
print(send(a, "REGISTER test7 123456"))
print(send(a, "LOGIN test7 123456"))

# Đóng kết nối trực tiếp, không gửi LOGOUT
a.close()

import time
time.sleep(0.3)

b = connect()
print(send(b, "LOGIN test7 123456"))
b.close()
PY
```

Kết quả mong đợi:

```text
OK Account registered
OK Login successful
OK Login successful
```

**Mục đích:** Kiểm tra server tự giải phóng phiên đăng nhập khi client mất kết nối mà không gửi `LOGOUT`.

Server thực hiện việc này trong `close_client()`:

```c
if (user_index >= 0 && user_index < user_count) {
    users[user_index].is_logged_in = 0;
    users[user_index].client_fd = -1;
}
```

## 8. Test 8 – QUIT giải phóng tài khoản

Tại client 1 đăng nhập `khanh` rồi nhập:

```text
QUIT
```

Kết quả mong đợi:

```text
OK Goodbye
```

Kết nối đóng. Sau đó tại client khác nhập:

```text
LOGIN khanh 123456
```

Kết quả mong đợi:

```text
OK Login successful
```

**Mục đích:** Kiểm tra `QUIT` đóng kết nối và giải phóng trạng thái đăng nhập.

> **Lưu ý:** Trong bản `server.c` hiện tại, `QUIT extra` (có tham số thừa) trả về lỗi nhưng vẫn bị đóng kết nối, còn `  QUIT` (có khoảng trắng đầu dòng) trả về `OK Goodbye` nhưng không đóng. Nên sửa trước khi coi lệnh `QUIT` là hoàn chỉnh.

## 9. Test 9 – Username và password quá dài

Trong code:

```c
#define USERNAME_SIZE 32
#define PASSWORD_SIZE 64
```

Username tối đa 31 ký tự, password tối đa 63 ký tự (dành một byte cho `\0`).

**Cách thực hiện:**

Tại Terminal 2, chạy:

```bash
python3 - <<'PY'
import socket

client = socket.create_connection(("127.0.0.1", 8080))
client.recv(1024)  # bỏ dòng chào

def send(line):
    client.sendall((line + "\n").encode())
    return client.recv(1024).decode().strip()

print("username 32 ky tu :", send("REGISTER " + "u" * 32 + " 123456"))
print("password 64 ky tu :", send("REGISTER user9 " + "p" * 64))
print("username 31 ky tu :", send("REGISTER " + "v" * 31 + " " + "p" * 63))

client.close()
PY
```

Kết quả mong đợi:

```text
username 32 ky tu : ERROR Username or password is too long
password 64 ky tu : ERROR Username or password is too long
username 31 ky tu : OK Account registered
```

**Mục đích:** Kiểm tra server từ chối dữ liệu vượt giới hạn, không tràn bộ đệm và không cắt chuỗi rồi đăng ký nhầm.

## 10. Test 10 – Chức năng cũ vẫn hoạt động

Nhập:

```text
PING
HELP
```

Kết quả mong đợi:

```text
OK PONG
OK Commands: HELP, PING, REGISTER, LOGIN, LOGOUT, QUIT
```

**Mục đích:** Kiểm tra việc thêm tài khoản không làm hỏng các lệnh của Commit 1 và Commit 2.

## 11. Tổng hợp 10 bài kiểm thử

| Test | Nội dung | Kết quả cần đạt |
| ---- | -------- | --------------- |
| 1 | Đăng ký và đăng ký trùng | `OK Account registered` / `ERROR Username already exists` |
| 2 | Đăng ký sai cú pháp | `ERROR Invalid command or arguments` |
| 3 | Đăng nhập đúng/sai | `OK Login successful` / `ERROR Invalid username or password` |
| 4 | Một client, hai tài khoản | `ERROR Already logged in` |
| 5 | Một tài khoản, hai client | `ERROR Account already logged in` |
| 6 | Đăng xuất | `OK Logout successful` / `ERROR Not logged in` |
| 7 | Client ngắt kết nối đột ngột | Tài khoản được giải phóng, đăng nhập lại được |
| 8 | `QUIT` | `OK Goodbye`, đóng kết nối, giải phóng tài khoản |
| 9 | Username/password quá dài | `ERROR Username or password is too long` |
| 10 | `PING`, `HELP` | Vẫn hoạt động |

## 12. Sau khi kiểm thử thành công

Biên dịch lại:

```bash
gcc -Wall -Wextra -std=c11 server.c -o server
```

Sau đó đưa mã nguồn và tài liệu kiểm thử vào Git (nhớ cách commit trước ít nhất 30 phút):

```bash
git add .
git commit -m "Add account registration, login and logout"
git push
```

### Mục tiêu Commit 3

Hoàn thiện đăng ký, đăng nhập, đăng xuất và quản lý phiên của từng client, làm nền cho Commit 4 (quản lý phòng đấu giá).

### Hạn chế còn lại

- Tài khoản lưu trong RAM, mất khi tắt server.
- Mật khẩu chưa được băm, đang lưu dạng chữ thường trong bộ nhớ.
- Chưa kiểm tra ký tự hợp lệ của username.
- Lệnh `QUIT` còn hai trường hợp biên như ở Test 8.

> **Lưu ý:** Đây là kế hoạch kiểm thử và kết quả mong đợi, chưa phải xác nhận 10 test đã chạy thành công trên máy của bạn. Chỉ đánh dấu Commit 3 hoàn thành sau khi bạn thực sự chạy và kiểm tra từng test.
