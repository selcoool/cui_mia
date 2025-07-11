#include <WiFi.h>

const char* ssid_const = "OK_AP";
const char* password_const = "12345678";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

void printMacAddress(uint8_t mac[6]) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.disconnect();
  delay(500);

  WiFi.config(local_ip, gateway, subnet);

  int channel = 6;
  char channelStr[4];
  snprintf(channelStr, sizeof(channelStr), "%d", channel);

  // Tạo mảng char để truyền vào apbegin
  char ssid[32], password[64];
  strncpy(ssid, ssid_const, sizeof(ssid));
  strncpy(password, password_const, sizeof(password));

  Serial.print("🚀 Bắt đầu phát Wi-Fi AP: ");
  Serial.print(ssid);
  Serial.print(" | Kênh: ");
  Serial.println(channelStr);

  int result = WiFi.apbegin(ssid, password, channelStr);  // Dùng đúng định dạng `char*` theo SDK Realtek

  if (result == WL_CONNECTED) {
    Serial.println("✅ Phát Wi-Fi thành công!");
    Serial.print("🔗 IP: ");
    Serial.println(WiFi.localIP());

    uint8_t mac[6];
    WiFi.macAddress(mac);  // Lấy MAC của interface hiện tại (AP)
    Serial.print("📡 MAC (AP): ");
    printMacAddress(mac);
  } else {
    Serial.println("❌ Không thể phát Wi-Fi. Mã lỗi: " + String(result));
    while (true) delay(1000);
  }
}

void loop() {
  // Không cần làm gì trong loop
}
