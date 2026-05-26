#include <Arduino.h>
#include <WiFi.h>

#include <AudioFileSourceHTTPStream.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

const char* ssid = "Le Thi Nam";
const char* password = "123456789@";

// MP3 URL
// const char* url =
//     "http://ice1.somafm.com/groovesalad-128-mp3";

const char* url = "http://s2-webradio.antenne.de/chillout";

AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioOutputI2S *out;

void setup() {

    Serial.begin(115200);

    WiFi.begin(ssid, password);

    Serial.print("Connecting");

    while (WiFi.status() != WL_CONNECTED) {

        delay(500);

        Serial.print(".");
    }

    Serial.println("");

    Serial.println("WiFi OK");

    out = new AudioOutputI2S();

    out->SetPinout(
        26,
        25,
        22
    );

    out->SetGain(0.7);

    file =
        new AudioFileSourceHTTPStream(
            url
        );

    mp3 = new AudioGeneratorMP3();

    if (mp3->begin(file, out)) {

        Serial.println("MP3 START");
    }
    else {

        Serial.println("MP3 FAIL");
    }
}

void loop() {

    if (mp3->isRunning()) {

        if (!mp3->loop()) {

            mp3->stop();

            Serial.println("MP3 DONE");
        }
    }
}