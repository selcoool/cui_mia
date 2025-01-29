#include <WiFi.h>
#include <WebSocketsClient.h>
#include "esp_camera.h"

// Wi-Fi credentials
const char* ssid = "D0806";
const char* password = "INSIDESD0806";

// WebSocket server IP (replace with your server IP)
const char* webSocketServer = "192.168.0.17"; // Node.js WebSocket Server IP

WebSocketsClient webSocket;

// Camera configuration
camera_config_t config;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Camera configuration setup (ensure correct pin mapping)
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12; // Low quality for faster transmission
  config.fb_count = 2;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Camera initialization failed!");
    return;
  }

  // Connect to WebSocket server on port 3001
  webSocket.begin(webSocketServer, 3001, "/");
  webSocket.onEvent(webSocketEvent); // Set event handler
}

void loop() {
  webSocket.loop(); // Keep checking WebSocket events

  // Capture and send image every 500ms
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 500) {
    lastTime = millis();

    // Capture frame from camera
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Send image frame via WebSocket
    webSocket.sendBIN(fb->buf, fb->len);
    Serial.println("Sent image to server");

    // Return the frame buffer to free memory
    esp_camera_fb_return(fb);
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
  }
}
