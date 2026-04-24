#include <Arduino.h>
#include <Preferences.h>


// ================= WEB =================
Preferences prefs;


void clearAll()
{
  prefs.begin("i2c", false);
  prefs.clear();
  prefs.end();

  prefs.begin("pid", false);
  prefs.clear();
  prefs.end();

  prefs.begin("pin", false);
  prefs.clear();
  prefs.end();

  prefs.begin("pulse_pwm", false);
  prefs.clear();
  prefs.end();


    prefs.begin("trim", false);
  prefs.clear();
  prefs.end();


  Serial.println("I2C + PID CLEARED ✔");
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  clearAll();

 
}

void loop() {
  
}