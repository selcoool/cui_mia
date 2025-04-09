#include <WiFi.h>

void setup() {
  Serial.begin(115200);  // Khởi tạo Serial Monitor
  WiFi.mode(WIFI_STA);  // Chế độ Wi-Fi station
  WiFi.disconnect();  // Ngắt kết nối bất kỳ nếu có
  
  Serial.println("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();  // Quét các mạng Wi-Fi

  if (n == 0) {
    Serial.println("No networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found:");
    for (int i = 0; i < n; ++i) {
      int channel = WiFi.channel(i);  // Lấy kênh của mạng Wi-Fi thứ i
      float frequency = 0.0;
      
      if (channel >= 1 && channel <= 13) {
        // Tính tần số cho dải 2.4 GHz
        frequency = 2.412 + (channel - 1) * 0.005;
      } else if (channel >= 36 && channel <= 165) {
        // Tính tần số cho dải 5 GHz
        frequency = 5.180 + (channel - 36) * 0.020;
      }

      Serial.print("SSID: ");
      Serial.print(WiFi.SSID(i));  // In SSID
      Serial.print(", Channel: ");
      Serial.print(channel);  // In kênh của mạng
      Serial.print(", Frequency: ");
      Serial.print(frequency);  // In tần số của mạng
      Serial.print(" GHz, RSSI: ");
      Serial.print(WiFi.RSSI(i));  // In cường độ tín hiệu RSSI
      Serial.print(", Encryption: ");
      printEncryptionType(WiFi.encryptionType(i)); // Loại mã hóa của mạng
      Serial.print(" | MAC (BSSID): ");
      Serial.println(WiFi.BSSIDstr(i));
    }
  }
}

void loop() {
  // Không cần làm gì trong vòng lặp
}


void printEncryptionType(int type) {
  switch (type) {
    case WIFI_AUTH_OPEN:
      Serial.println("Open");
      break;
    case WIFI_AUTH_WEP:
      Serial.println("WEP");
      break;
    case WIFI_AUTH_WPA_PSK:
      Serial.println("WPA PSK");
      break;
    case WIFI_AUTH_WPA2_PSK:
      Serial.println("WPA2 PSK");
      break;
    case WIFI_AUTH_WPA_WPA2_PSK:
      Serial.println("WPA/WPA2 PSK");
      break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
      Serial.println("WPA2 Enterprise");
      break;
    case WIFI_AUTH_WPA3_PSK:
      Serial.println("WPA3 PSK");
      break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
      Serial.println("WPA2/WPA3 PSK");
      break;
    default:
      Serial.println("Unknown");
  }

}
