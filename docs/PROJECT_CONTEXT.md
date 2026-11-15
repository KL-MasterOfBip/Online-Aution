# PROJECT_CONTEXT - ONLINE AUCTION (C)

## 1. Mục tiêu

Xây dựng ứng dụng đấu giá trực tuyến bằng ngôn ngữ C, sử dụng TCP Socket.
Phát triển theo từng chức năng nhỏ, mỗi phần phải biên dịch và chạy được trước
khi chuyển sang phần tiếp theo. Mã nguồn đơn giản, phù hợp với các bài thực hành
lập trình mạng C đã làm.

Deadline: 21/12/2026.

## 2. Phong cách và kỹ thuật đã thống nhất

- Ngôn ngữ: C.
- Giao tiếp: TCP Socket.
- Server dùng `select()` để xử lý nhiều client trong một tiến trình.
- Ưu tiên code đơn giản: `socket`, `bind`, `listen`, `accept`, `select`,
  `recv`, `send`, `close`.
- Không tự ý chuyển sang kiến trúc phức tạp như epoll, thread pool hoặc microservice.
- Giao thức ứng dụng dùng văn bản, mỗi thông điệp kết thúc bằng `\\n`.
- TCP là luồng byte: phải xử lý trường hợp một lệnh bị chia qua nhiều lần `recv()`
  hoặc nhiều lệnh đến trong cùng một lần `recv()`.
- Server là nơi kiểm tra quyền và cập nhật trạng thái; không tin dữ liệu client gửi lên.
- Mỗi giai đoạn cần có hướng dẫn build, chạy, test thủ công và nội dung commit.
- Không tuyên bố đã test nếu chưa thực sự chạy test.

## 3. Yêu cầu chức năng cuối cùng

1. Đăng ký và quản lý tài khoản.
2. Đăng nhập, đăng xuất và quản lý phiên.
3. Tạo phòng đấu giá.
4. Liệt kê phòng đấu giá.
5. Mỗi tài khoản chỉ được tham gia tối đa một phòng tại một thời điểm.
6. Người tạo phòng có thể tạo vật phẩm gồm:
   - Tên/thông tin vật phẩm.
   - Giá khởi điểm.
   - Giá mua ngay.
7. Chỉ được rút/xóa vật phẩm khi vật phẩm chưa được đưa ra đấu giá.
8. Các vật phẩm được đưa ra đấu giá lần lượt.
9. Đặt giá phải cao hơn giá hiện tại ít nhất 10.000 đồng.
10. Giá hợp lệ phải được thông báo đến các thành viên trong phòng.
11. Sau mỗi lần đặt giá, nếu không có giá cao hơn trong 3 phút thì server
    gửi thông báo 3 lần, mỗi lần cách nhau một khoảng thời gian.
12. Mua ngay vật phẩm mà không cần đấu giá.
13. Ghi log hoạt động.

## 4. Nguyên tắc xử lý nghiệp vụ

- Một phòng chỉ có một phiên đấu giá đang diễn ra tại một thời điểm.
- Một vật phẩm chỉ được bán một lần.
- Người bán không được tự trả giá cho vật phẩm của mình.
- Giá đặt mới phải thỏa mãn:
  `new_bid >= current_price + 10000`.
- Giá mua ngay phải được kiểm tra ở server và kết thúc vật phẩm ngay khi giao dịch hợp lệ.
- Khi có giá mới, server cập nhật giá hiện tại trước rồi mới broadcast cho phòng.
- Thời gian đấu giá được quản lý bởi server, không dựa vào đồng hồ client.
- Chỉ thành viên hiện tại của phòng mới nhận được broadcast của phòng.
- Cần xác định rõ quy tắc người bán có được tham gia trả giá hay không trong báo cáo;
  mặc định triển khai là không được tự trả giá cho vật phẩm của mình.

## 5. Kế hoạch tuần đầu - 5 commit

Mỗi commit phải cách commit trước ít nhất 30 phút. Không tạo commit giả hoặc
thay đổi timestamp để đối phó yêu cầu. Mỗi thành viên nhóm cần có commit thực
chất trên repository.

| Commit | Nội dung | Kết quả cần chạy được |
|---|---|---|
| 1 | Khởi tạo project, TCP server, `select()`, xử lý client và framing theo dòng | Nhiều client kết nối; HELP/PING/QUIT hoạt động |
| 2 | Chuẩn hóa giao thức lệnh và xử lý lỗi đầu vào | Lệnh sai, dòng rỗng, lệnh dài, nhiều lệnh trong một lần gửi được xử lý |
| 3 | Đăng ký, đăng nhập, đăng xuất và session trong bộ nhớ | Hai client tạo tài khoản/đăng nhập độc lập |
| 4 | Tạo phòng, liệt kê phòng, tham gia/rời phòng | Một user không thể ở hai phòng cùng lúc |
| 5 | Ghi log hoạt động, bổ sung unit test cho các phần đã có, cập nhật README | Có log và lệnh chạy test; các chức năng tuần đầu được kiểm tra |

Commit 1 là mã nguồn hiện tại. Không đưa chức năng chưa hoàn thành vào commit
chỉ để đủ số lượng.

## 6. Lộ trình dự kiến đến deadline

- Tuần 1: Nền tảng TCP, framing, tài khoản/session, phòng đấu giá, log cơ bản.
- Tuần 2: Quản lý vật phẩm: tạo, liệt kê, rút vật phẩm chưa mở phiên; kiểm soát chủ phòng.
- Tuần 3: Phiên đấu giá tuần tự, đặt giá, mức tăng tối thiểu, broadcast.
- Tuần 4: Mua ngay, bộ đếm 3 phút, thông báo kết thúc và xử lý các tình huống cạnh tranh.
- Tuần 5: Hoàn thiện lưu trữ dữ liệu, unit test, coverage trên 90%, kiểm thử tải,
  sửa lỗi và chuẩn bị mã nguồn nộp.

Lộ trình có thể điều chỉnh theo tiến độ thực tế. Các yêu cầu bắt buộc cuối cùng
không được bỏ sót.

## 7. Yêu cầu kiểm thử và nộp bài

- Public GitHub repository.
- Unit test với coverage trên 90%; cần thống nhất phạm vi đo coverage và loại trừ
  phần mã không thể kiểm thử hợp lý nếu được giảng viên cho phép.
- Có bài kiểm tra khả năng chịu tải: nhiều client kết nối, gửi lệnh đồng thời,
  đo số client xử lý được, tỷ lệ lỗi và thời gian phản hồi.
- Nộp báo cáo, slide và mã nguồn. Trước mắt ưu tiên mã nguồn; chưa làm slide.
- Mỗi tuần tối thiểu 5 commit; mỗi commit cách nhau tối thiểu 30 phút.
- Các thành viên đều phải có commit.

## 8. Trạng thái hiện tại

### Đã có
- `src/server.c`: TCP server dùng `select()`.
- Nhiều client có buffer riêng.
- Xử lý lệnh theo dòng `\\n`, kể cả khi dữ liệu đến thành nhiều phần.
- Lệnh `HELP`, `PING`, `QUIT`.
- `Makefile`, `README.md`.

### Chưa có
- Tài khoản và session.
- Phòng đấu giá.
- Vật phẩm.
- Đặt giá và mua ngay.
- Timer đấu giá.
- Log nghiệp vụ.
- Unit test và load test.
- Lưu dữ liệu bền vững.

## 9. Quy tắc cập nhật PROJECT_CONTEXT

Sau mỗi buổi làm việc, cập nhật:
1. Commit đã hoàn thành và hash nếu người dùng cung cấp.
2. File đã thêm/sửa.
3. Cách build và test đã chạy thực tế.
4. Lỗi còn tồn tại.
5. Việc tiếp theo.
6. Điều chỉnh kế hoạch commit nhưng vẫn giữ đủ yêu cầu cuối cùng.

Không tự đánh dấu chức năng là hoàn thành chỉ vì đã viết code; phải có cách kiểm tra
và kết quả chạy thực tế.
