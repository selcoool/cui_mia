#include <WiFi.h>

const char* ssid_const = "Deauther_AP";
const char* password_const = "12345678";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.disconnect();
  delay(500);

  WiFi.config(local_ip, gateway, subnet);

  int channel = 6;

  // Tạo mảng char có thể ghi
  char ssid[32], password[64], channelStr[4];
  strncpy(ssid, ssid_const, sizeof(ssid));
  strncpy(password, password_const, sizeof(password));
  snprintf(channelStr, sizeof(channelStr), "%d", channel);  // chuyển số sang chuỗi

  Serial.print("🚀 Bắt đầu phát Wi-Fi AP: ");
  Serial.print(ssid);
  Serial.print(" | Kênh: ");
  Serial.println(channelStr);

  int result = WiFi.apbegin(ssid, password, channelStr);  // Truyền đúng kiểu `char*`

  if (result == WL_CONNECTED) {
    Serial.println("✅ Phát Wi-Fi thành công!");
    Serial.print("🔗 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("❌ Không thể phát Wi-Fi. Mã lỗi: " + String(result));
    while (true) delay(1000);
  }
}

void loop() {
  // Code khác nếu cần
}