const WebSocket = require('ws');
const multer = require('multer');
const path = require('path');
const fs = require('fs');

// Thiết lập thư mục lưu trữ bằng multer
const uploadDir = path.join(__dirname, 'upload');
if (!fs.existsSync(uploadDir)) {
  fs.mkdirSync(uploadDir);
}

// Cấu hình multer
const storage = multer.diskStorage({
  destination: (req, file, cb) => {
    cb(null, uploadDir);
  },
  filename: (req, file, cb) => {
    const timestamp = Date.now();
    cb(null, `image_${timestamp}${path.extname(file.originalname)}`);
  }
});
const upload = multer({ storage });

// Khởi tạo WebSocket server tại cổng 3001
const wss = new WebSocket.Server({ port: 3001 }, () => {
  console.log('WebSocket server is running at ws://localhost:3001');
});

// Khi có client (ESP32) kết nối
wss.on('connection', ws => {
  console.log('Client connected.');

  // Lắng nghe và nhận dữ liệu ảnh từ ESP32
  ws.on('message', (message) => {
    console.log('Received image data');

    // Lưu ảnh bằng multer
    const timestamp = Date.now();
    const filename = `image_${timestamp}.jpg`;
    const filePath = path.join(uploadDir, filename);

    // Ghi dữ liệu ảnh vào file
    fs.writeFile(filePath, message, (err) => {
      if (err) {
        console.error('Error saving image:', err);
      } else {
        console.log(`Image saved as ${filePath}`);
      }
    });
  });

  // Gửi thông điệp chào mừng đến ESP32
  ws.send('Hello from WebSocket Server!');
});
