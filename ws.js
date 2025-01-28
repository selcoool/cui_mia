const WebSocket = require('ws');
const fs = require('fs');

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

    // Lưu dữ liệu hình ảnh vào file
    const timestamp = Date.now();
    const filename = `image_${timestamp}.jpg`;  // Tên file ảnh

    // Ghi dữ liệu hình ảnh vào file
    fs.writeFile(filename, message, (err) => {
      if (err) {
        console.error('Error writing image:', err);
      } else {
        console.log(`Image saved as ${filename}`);
      }
    });
  });

  // Gửi thông điệp chào mừng đến ESP32
  ws.send('Hello from WebSocket Server!');
});