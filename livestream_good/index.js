const WebSocket = require('ws');
const express = require('express');
const cors = require('cors'); // Thêm dòng này
const path = require('path');

const app = express();
const PORT_HTTP = 3000; // Port cho HTTP server
const PORT_WS = 3001; // Port cho WebSocket server

// Cấu hình CORS để cho phép truy cập từ tất cả các thiết bị
app.use(cors());

// Cấu hình để phục vụ file tĩnh (HTML client)
app.use(express.static(path.join(__dirname, 'public')));

// Khởi chạy HTTP server
app.listen(PORT_HTTP, () => {
  console.log(`HTTP server running at http://0.0.0.0:${PORT_HTTP}`);
});

// Tạo WebSocket server
const wss = new WebSocket.Server({ port: PORT_WS }, () => {
  console.log(`WebSocket server running at ws://0.0.0.0:${PORT_WS}`);
});

// Xử lý kết nối WebSocket
wss.on('connection', (ws) => {
  console.log('ESP32 connected');

  // Lắng nghe khi nhận dữ liệu từ ESP32
  ws.on('message', (data) => {
    console.log('Received frame of size:', data.length);

    // Gửi dữ liệu tới tất cả các client WebSocket
    wss.clients.forEach(client => {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(data);
      }
    });
  });

  // Khi ESP32 đóng kết nối
  ws.on('close', () => {
    console.log('ESP32 disconnected');
  });

  // Xử lý lỗi
  ws.on('error', (error) => {
    console.error('WebSocket error:', error);
  });
});
