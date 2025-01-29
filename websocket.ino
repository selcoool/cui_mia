#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>  // Include ArduinoJson library for JSON handling

// Wi-Fi credentials
const char* ssid = "D0806";
const char* password = "INSIDESD0806";

// WebSocket server IP (replace with your server IP)
const char* webSocketServer = "192.168.0.17"; // Node.js WebSocket Server IP

WebSocketsClient webSocket;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Connect to WebSocket server on port 3001
  webSocket.begin(webSocketServer, 3001, "/");
  webSocket.onEvent(webSocketEvent); // Set event handler
}

void loop() {
  webSocket.loop(); // Keep checking WebSocket events

  // Send a message to the server every 15 seconds
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 15000) {
    lastTime = millis();

    // Create a JSON object
    DynamicJsonDocument doc(1024);
    doc["event"] = "newMessage";
    doc["data"] = "Hello from ESP32 client!";

    // Convert JSON object to string
    String message;
    serializeJson(doc, message);

    // Send JSON message to server
    webSocket.sendTXT(message); // Send message to server
    Serial.println("Sent message to server: " + message);
  }
}

// WebSocket event handler
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_CONNECTED) {
    // Connection established
    Serial.println("Connected to WebSocket server successfully!");
    
    // Send a message after successful connection
    String message = "ESP32 client: Connection successful!";
    webSocket.sendTXT(message);  
    Serial.println("Sent message to server: " + message);
  } 
  else if (type == WStype_DISCONNECTED) {
    Serial.println("Disconnected from WebSocket server.");
  }
  else if (type == WStype_ERROR) {
    Serial.println("WebSocket error occurred.");
  }
  else if (type == WStype_TEXT) {
    // Handle incoming messages from the server
    String receivedMessage = String((char*)payload);
    Serial.println("Received message from server: " + receivedMessage);
    
    // Example of what to do with the response (if it contains "Connection successful")
    if (receivedMessage.indexOf("Connection successful") != -1) {
      Serial.println("Successfully connected to the server! Server says: " + receivedMessage);
    }
  }
}
