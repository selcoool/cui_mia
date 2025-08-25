#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Starting WiFi scan...");
}

void loop() {
  int n = WiFi.scanNetworks();

  if (n == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.print("Found ");
    Serial.print(n);
    Serial.println(" networks:");

    for (int i = 0; i < n; i++) {
      Serial.print(i + 1);
      Serial.print(": SSID: ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" | BSSID: ");

      uint8_t* bssid = WiFi.BSSID(i);
      for (int j = 0; j < 6; j++) {
        if (bssid[j] < 16) Serial.print("0");
        Serial.print(bssid[j], HEX);
        if (j < 5) Serial.print(":");
      }

      Serial.print(" | RSSI: ");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm");
    }
  }

  Serial.println("------------------------");
  delay(10000); // scan lại sau 10 giây
}
