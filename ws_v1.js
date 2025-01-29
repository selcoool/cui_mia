const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');

// Thư mục để lưu ảnh
const uploadDir = path.join(__dirname, 'upload');

// Kiểm tra và tạo thư mục nếu chưa tồn tại
if (!fs.existsSync(uploadDir)) {
  fs.mkdirSync(uploadDir);
}

// Khởi tạo WebSocket server tại cổng 3001
const wss = new WebSocket.Server({ port: 3001 }, () => {
  console.log('WebSocket server is running at ws://localhost:3001');
});

// Khi có client (ESP32) kết nối
wss.on('connection', ws => {
  console.log('Client connected.');

  // Lắng nghe và nhận hình ảnh từ ESP32
  ws.on('message', (message) => {
    console.log('Received image data');

    // Tạo tên file với timestamp
    const timestamp = Date.now();
    const filename = `image_${timestamp}.jpg`;

    // Đường dẫn đầy đủ đến file
    const filePath = path.join(uploadDir, filename);

    // Ghi dữ liệu hình ảnh vào file
    fs.writeFile(filePath, message, (err) => {
      if (err) {
        console.error('Error writing image:', err);
      } else {
        console.log(`Image saved as ${filePath}`);
      }
    });
  });

  // Gửi thông điệp chào mừng đến ESP32
  ws.send('Hello from WebSocket Server!');
});
