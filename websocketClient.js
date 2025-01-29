const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');

// Khởi tạo WebSocket server
const wss = new WebSocket.Server({ port: 3001 });

// Thiết lập thư mục lưu ảnh
const uploadDir = './uploads';
if (!fs.existsSync(uploadDir)) {
  fs.mkdirSync(uploadDir);
}

// Lắng nghe kết nối WebSocket
wss.on('connection', (ws) => {
  console.log('Client connected');

  // Xử lý khi nhận dữ liệu từ ESP32
  ws.on('message', (data) => {
    console.log('Received image frame from ESP32');
    
    // Lưu ảnh vào thư mục uploads
    const timestamp = Date.now();
    const imagePath = path.join(uploadDir, `image_${timestamp}.jpg`);
    fs.writeFile(imagePath, data, (err) => {
      if (err) {
        console.error('Error saving image:', err);
      } else {
        console.log('Image saved as', imagePath);
      }
    });
  });

  ws.on('close', () => {
    console.log('Client disconnected');
  });

  ws.on('error', (err) => {
    console.error('WebSocket error:', err);
  });
});

console.log('WebSocket server running on ws://localhost:3001');
