const WebSocket = require('ws');
const express = require('express');
const cors = require('cors'); // Thêm dòng này để khai báo cors

const app = express();
const wss = new WebSocket.Server({ port: 3001 });

app.use(cors({ origin: "*" }));

// Endpoint để phục vụ HTML Client
app.get('/', (req, res) => {
  res.sendFile(__dirname + '/public/index.html');
});

// Kết nối WebSocket
wss.on('connection', (ws) => {
  console.log('ESP32 connected');

  ws.on('message', (data) => {
    console.log('Received frame of size:', data.length);

    // Gửi dữ liệu tới tất cả các client WebSocket
    wss.clients.forEach(client => {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(data);
      }
    });
  });

  ws.on('close', () => {
    console.log('ESP32 disconnected');
  });
});

// Chạy server
app.listen(3002, () => {
  console.log('Server running on http://localhost:3000');
});
