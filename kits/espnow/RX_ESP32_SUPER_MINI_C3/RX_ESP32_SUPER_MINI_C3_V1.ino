
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>


int M1 = 3;
int M2 = 6;
int M3 = 5;
int M4 = 4;


int SM1 = 20;  // motor 1 chậm
int SM2 = 50;  // motor 2 nhanh hơn
int SM3 = 50;  // motor 3 nhanh hơn nữa
int SM4 = 50;  // motor 4 tối đa


// ================= DATA =================
typedef struct {
  uint16_t ch[8];
} Data;

Data rx;

// ================= STATE =================
unsigned long lastRX = 0;

bool connected = false;
bool armed = false;

uint8_t lostCount = 0;

// ================= ESP-NOW CALLBACK =================
void onRecv(const uint8_t *mac, const uint8_t *data, int len)
{
    memcpy(&rx, data, sizeof(rx));

    lastRX = millis();
    connected = true;
    lostCount = 0;
}

// ================= SETUP =================
void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW FAIL");
        return;
    }

    esp_now_register_recv_cb(onRecv);


    pinMode(M1, OUTPUT);
    pinMode(M2, OUTPUT);
    pinMode(M3, OUTPUT);
    pinMode(M4, OUTPUT);

    Serial.println("RX READY");
}

// ================= LOOP =================
void loop()
{


  // analogWrite(M1, SM1);
  // analogWrite(M2, SM2);
  // analogWrite(M3, SM3);
  // analogWrite(M4, SM4);  


    // ================= CHECK LOST TX =================
    if (connected && millis() - lastRX > 300)
    {
        lostCount++;

        if (lostCount >= 5)
        {
            connected = false;

            if (armed)
            {
                armed = false;
                Serial.println("AUTO DISARM");
            }

            Serial.println("TX DISCONNECTED");
        }

        lastRX = millis();
    }

    // ================= ARM / DISARM =================
    if (connected)
    {

          //         // ARM  khi giam xuong
      if (!armed && rx.ch[4] < 1000)
          {
              armed = true;



             

              Serial.println("ARM");


                analogWrite(M1, SM1);
                analogWrite(M2, SM2);
                analogWrite(M3, SM3);
                analogWrite(M4, SM4);  
          }

          // DISARM
          if (armed && rx.ch[4] > 1800)
          {
              armed = false;
              Serial.println("DISARM");

              analogWrite(M1, 0);
              analogWrite(M2, 0);
              analogWrite(M3, 0);
              analogWrite(M4, 0);  
          }

      //         // ARM  khi tang len
      // if (!armed && rx.ch[4] > 1800)
      // {
      //     armed = true;
      //     Serial.println("ARM");
      // }

      // // DISARM
      // if (armed && rx.ch[4] < 1000)
      // {
      //     armed = false;
      //     Serial.println("DISARM");
      // }
       


       
    }

    // ================= DEBUG =================
    static unsigned long t = 0;
    if (millis() - t > 500)
    {
        t = millis();

        Serial.print("Connected: ");
        Serial.print(connected);

        Serial.print("  Armed: ");
        Serial.print(armed);

        Serial.print("  CH5: ");
        Serial.println(rx.ch[4]);
       
    }


      delay(500); // 0.5s
}
