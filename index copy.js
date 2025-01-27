const WebSocket = require('ws');
const express = require('express');

// Initialize the Express app
const app = express();

// Create a WebSocket server using the ws library
const wss = new WebSocket.Server({ port: 3001 });

// Handle incoming WebSocket connections
wss.on('connection', (ws) => {
  console.log('ESP32 client connected');

  // Send a message to the client when it connects
  ws.send(JSON.stringify({ message: 'Connection successful from Node.js server!' }));

  // Handle incoming messages from the client
  ws.on('message', (message) => {
    // Convert message (Buffer) to string if it's in Buffer format
    if (Buffer.isBuffer(message)) {
      message = message.toString();  // Convert Buffer to string
    }
    console.log('Message from ESP32 client:', message);

    // Respond back to the ESP32 client
    ws.send(JSON.stringify({ message: 'Message received by Node.js server: ' + message }));
  });

  // Handle client disconnection
  ws.on('close', () => {
    console.log('ESP32 client disconnected');
  });
});

// Start the server
app.listen(3002, () => {
  console.log('Node.js WebSocket server is running on ws://localhost:3001');
});
