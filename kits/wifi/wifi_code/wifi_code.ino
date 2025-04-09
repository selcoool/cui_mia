#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_system.h"

// Định nghĩa callback cho chế độ promiscuous
wifi_promiscuous_cb_t promiscuous_cb;

void promiscuous_cb_handler(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    
    // In ra thông tin chi tiết về các gói nhận được
    Serial.print("Packet Type: ");
    Serial.println(type);

    // Kiểm tra các gói deauthentication
    if (pkt->payload[0] == 0xC0) {  // Xác định loại frame deauthentication
        Serial.println("Deauthentication frame detected.");
    } else {
        // In toàn bộ byte của gói tin
        Serial.print("Received Packet: ");
        
        // Lấy độ dài của gói tin từ đầu đến hết
        int len = pkt->rx_ctrl.sig_len;  // Thường thì sig_len cho biết độ dài gói tin
        
        for (int i = 0; i < len; i++) {
            Serial.print(pkt->payload[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // Chế độ station (STA)
  
  // Bật chế độ promiscuous
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(promiscuous_cb_handler);  // Đặt callback cho chế độ promiscuous

  Serial.println("Waiting for deauth frames...");
  
  // Đảm bảo là ESP32 đang hoạt động trên kênh Wi-Fi mà mạng của bạn sử dụng
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);  // Ví dụ, kênh 6
  
  // Tùy chọn: đợi một khoảng thời gian trước khi tiếp tục (nếu cần)
  delay(1000);
}

void loop() {
  // Chế độ promiscuous sẽ tự động lắng nghe và nhận các gói tin, không cần mã trong loop()
  delay(1000);
}
