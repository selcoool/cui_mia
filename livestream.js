const express = require('express');
const WebSocket = require('ws');
const app = express();
const port = 8080;

// WebSocket server (để nhận khung hình từ ESP32)
const wss = new WebSocket.Server({ port: 3001 });

let clients = [];
let currentFrame = null;  // Dùng để lưu trữ frame video mới nhất

// Khi có client kết nối tới WebSocket server (ESP32)
wss.on('connection', (ws) => {
  clients.push(ws);
  console.log('New client connected');

  ws.on('message', (message) => {
    // Khi nhận được hình ảnh từ ESP32 (bạn có thể điều chỉnh logic nhận hình ảnh ở đây)
    console.log('Received frame from ESP32');
    currentFrame = message;  // Giả sử message là dữ liệu hình ảnh JPEG
  });

  ws.on('close', () => {
    console.log('Client disconnected');
    clients = clients.filter(client => client !== ws);
  });
});

// Cung cấp MJPEG stream tại /video
app.get('/video', (req, res) => {
  res.setHeader('Content-Type', 'multipart/x-mixed-replace; boundary=frame');

  // Gửi hình ảnh MJPEG tới client mỗi khi có hình mới từ ESP32
  const sendFrame = () => {
    if (currentFrame) {
      // Gửi mỗi frame dưới dạng MJPEG
      res.write(`--frame\r\n`);
      res.write('Content-Type: image/jpeg\r\n\r\n');
      res.write(currentFrame);
      res.write('\r\n');
    }
  };

  // Đảm bảo rằng chúng ta gửi hình ảnh mỗi khi có frame mới từ ESP32
  const intervalId = setInterval(() => {
    sendFrame();
  },20); // Gửi hình ảnh mỗi 100ms

  // Dừng stream khi client đóng kết nối
  req.on('close', () => {
    clearInterval(intervalId);
    console.log('Client closed the video stream');
  });
});

app.listen(port, () => {
  console.log(`Server is running at http://localhost:${port}`);
});
