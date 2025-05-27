#include <SPI.h>
#include <RF24.h>

#define CE_PIN 7
#define CSN_PIN 10

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

char receivedText[64]; // Buffer to hold received message

bool connected = false; // Flag to track connection status

void setup() {
  Serial.begin(115200);
  
  if (!radio.begin()) {
    Serial.println("❌ NRF24 không phát hiện được!");
    while (1); // Stop execution if module fails to initialize
  }

  radio.openReadingPipe(0, address);   // Open reading pipe
  radio.setPALevel(RF24_PA_LOW);       // Set power level
  radio.setDataRate(RF24_1MBPS);       // Set data rate
  radio.startListening();              // Start listening for data

  Serial.println("📡 Đang chờ dữ liệu...");

  connected = true; // Set flag to true after successful initialization
}

void loop() {
  if (connected) {
    Serial.println("✅ Connected successfully");  // Display the connection message continuously
    connected = false; // To avoid repeating the message continuously in the loop
  }

  if (radio.available()) {
    radio.read(&receivedText, sizeof(receivedText)); // Read the incoming message

    // Print the received data for debugging
    Serial.print("📥 Dữ liệu nhận được: ");
    Serial.println(receivedText);

    // Parse joystick values from the received message
    int joy1X = getValue(receivedText, "q").toInt();
    int joy1Y = getValue(receivedText, "w").toInt();
    int joy2X = getValue(receivedText, "e").toInt();
    int joy2Y = getValue(receivedText, "f").toInt();

    // Print joystick values
    Serial.print("🎮 Joystick 1 - X: ");
    Serial.print(joy1X);
    Serial.print(", Y: ");
    Serial.println(joy1Y);

    Serial.print("🎮 Joystick 2 - X: ");
    Serial.print(joy2X);
    Serial.print(", Y: ");
    Serial.println(joy2Y);
  }
}

// Function to extract value based on key (e.g., "q", "w", etc.)
String getValue(String data, String key) {
    int startIndex = data.indexOf(key + ":");
    if (startIndex == -1) return "";  // Return empty string if key is not found
    startIndex += key.length() + 1;  // Move to the start of the value
    int endIndex = data.indexOf(" ", startIndex);  // Find the space or end of string
    if (endIndex == -1) endIndex = data.length();  // If no space, use the end of the string
    return data.substring(startIndex, endIndex); // Extract the value
}
