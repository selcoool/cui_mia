#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  Serial.printf("🔸 AVRC metadata received: %s\n", text);
}

void setup() {
  Serial.begin(115200);
  Serial.println("🔊 ESP32 Bluetooth Speaker Starting...");

  // Callback khi kết nối hoặc ngắt kết nối (gộp lại)
  a2dp_sink.set_connected([](bool connected, const char* name) {
    if (connected) {
      Serial.print("✅ Connected to: ");
      Serial.println(name);
    } else {
      Serial.print("❌ Disconnected from: ");
      Serial.println(name);
    }
  });

  // Callback khi nhận metadata (tên bài hát, artist...)
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);

  // Khởi động Bluetooth A2DP
  a2dp_sink.start("ESP32-Speaker");
  Serial.println("📡 Waiting for connection...");
}

void loop() {
  // không cần làm gì trong loop
}
