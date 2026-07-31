#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 8
#define SCL_PIN 9

#define MPU_ADDR 0x68
#define AK_ADDR  0x0C
#define BMP_ADDR 0x76

uint8_t readReg(uint8_t addr, uint8_t reg)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);

    if (Wire.endTransmission(false) != 0)
        return 0xFF;

    if (Wire.requestFrom(addr, (uint8_t)1) != 1)
        return 0xFF;

    return Wire.read();
}

bool exists(uint8_t addr)
{
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);

    Serial.println();
    Serial.println("================================");
    Serial.println("     SENSOR IDENTIFICATION");
    Serial.println("================================");

    // ==========================================
    // MPU
    // ==========================================

    if (exists(MPU_ADDR))
    {
        Serial.println();
        Serial.println("I2C 0x68: FOUND");

        for (int i = 0; i < 10; i++)
        {
            uint8_t who = readReg(
                MPU_ADDR,
                0x75
            );

            Serial.print("WHO_AM_I [");
            Serial.print(i);
            Serial.print("] = 0x");

            if (who < 0x10)
                Serial.print("0");

            Serial.println(who, HEX);

            delay(100);
        }

        uint8_t who = readReg(
            MPU_ADDR,
            0x75
        );

        Serial.println();

        if (who == 0x68)
        {
            Serial.println(">>> MPU6050");
        }
        else if (who == 0x70)
        {
            Serial.println(">>> MPU6500");
        }
        else if (who == 0x71)
        {
            Serial.println(">>> MPU9250");
        }
        else if (who == 0x73)
        {
            Serial.println(">>> MPU9255");
        }
        else
        {
            Serial.println(">>> UNKNOWN");
        }
    }
    else
    {
        Serial.println("I2C 0x68: NOT FOUND");
    }

    // ==========================================
    // AK8963
    // ==========================================

    Serial.println();

    if (exists(AK_ADDR))
    {
        Serial.println("I2C 0x0C: FOUND");

        uint8_t who = readReg(
            AK_ADDR,
            0x00
        );

        Serial.print(
            "AK8963 WHO_AM_I = 0x"
        );

        Serial.println(
            who,
            HEX
        );

        if (who == 0x48)
        {
            Serial.println(
                ">>> AK8963 MAGNETOMETER"
            );
        }
    }
    else
    {
        Serial.println(
            "I2C 0x0C: NOT FOUND"
        );
    }

    // ==========================================
    // BMP280
    // ==========================================

    Serial.println();

    if (exists(BMP_ADDR))
    {
        uint8_t id = readReg(
            BMP_ADDR,
            0xD0
        );

        Serial.print(
            "I2C 0x76: FOUND, CHIP ID = 0x"
        );

        Serial.println(
            id,
            HEX
        );

        if (id == 0x58)
        {
            Serial.println(
                ">>> BMP280"
            );
        }
        else if (id == 0x60)
        {
            Serial.println(
                ">>> BME280"
            );
        }
        else
        {
            Serial.println(
                ">>> UNKNOWN BMP/BME"
            );
        }
    }
    else
    {
        Serial.println(
            "I2C 0x76: NOT FOUND"
        );
    }

    Serial.println();
    Serial.println("================================");
    Serial.println("             DONE");
    Serial.println("================================");
}

void loop()
{
}
