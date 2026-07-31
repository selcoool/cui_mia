#include <Arduino.h>
#include <Wire.h>

// =====================================================
// ESP32-C3 SUPER MINI
// =====================================================

#define SDA_PIN 8
#define SCL_PIN 9

// =====================================================
// I2C ADDRESS
// =====================================================

#define MPU_ADDR       0x68
#define AK8963_ADDR    0x0C

// =====================================================
// READ REGISTER
// =====================================================

uint8_t readReg(
    uint8_t addr,
    uint8_t reg
)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);

    if (Wire.endTransmission(false) != 0)
        return 0xFF;

    if (Wire.requestFrom(addr, (uint8_t)1) != 1)
        return 0xFF;

    if (Wire.available())
        return Wire.read();

    return 0xFF;
}

// =====================================================
// WRITE REGISTER
// =====================================================

bool writeReg(
    uint8_t addr,
    uint8_t reg,
    uint8_t value
)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value);

    return Wire.endTransmission() == 0;
}

// =====================================================
// CHECK ADDRESS
// =====================================================

bool exists(uint8_t addr)
{
    Wire.beginTransmission(addr);

    return Wire.endTransmission() == 0;
}

// =====================================================
// PRINT ADDRESS
// =====================================================

void printAddress(uint8_t addr)
{
    Serial.print("0x");

    if (addr < 0x10)
        Serial.print("0");

    Serial.print(addr, HEX);
}

// =====================================================
// MPU IDENTIFICATION
// =====================================================

String identifyMPU()
{
    uint8_t id =
        readReg(
            MPU_ADDR,
            0x75
        );

    if (id == 0x68)
        return "MPU6050";

    if (id == 0x70)
        return "MPU6500";

    if (id == 0x71)
        return "MPU9250";

    if (id == 0x73)
        return "MPU9255";

    return "Unknown MPU";
}

// =====================================================
// BMP / BME IDENTIFICATION
// =====================================================

String identifyBMP(
    uint8_t addr
)
{
    uint8_t id =
        readReg(
            addr,
            0xD0
        );

    if (id == 0x56 ||
        id == 0x57 ||
        id == 0x58)
    {
        return "BMP280";
    }

    if (id == 0x60)
    {
        return "BME280";
    }

    if (id == 0x61)
    {
        return "BME680";
    }

    return "Unknown BMP/BME";
}

// =====================================================
// AK8963
// =====================================================

bool checkAK8963()
{
    if (!exists(AK8963_ADDR))
        return false;

    uint8_t id =
        readReg(
            AK8963_ADDR,
            0x00
        );

    return id == 0x48;
}

// =====================================================
// BẬT MPU BYPASS
// =====================================================

void enableMPUBypass()
{
    if (!exists(MPU_ADDR))
        return;

    uint8_t id =
        readReg(
            MPU_ADDR,
            0x75
        );

    // Chỉ làm với MPU9250 / MPU9255
    if (id != 0x71 &&
        id != 0x73)
    {
        return;
    }

    // Wake up
    writeReg(
        MPU_ADDR,
        0x6B,
        0x00
    );

    delay(100);

    // I2C bypass
    writeReg(
        MPU_ADDR,
        0x37,
        0x02
    );

    delay(50);
}

// =====================================================
// NHẬN DẠNG THIẾT BỊ
// =====================================================

void identifyDevice(
    uint8_t addr
)
{
    Serial.print("  ");
    printAddress(addr);

    Serial.print(" -> ");

    // =================================================
    // MPU
    // =================================================

    if (addr == 0x68)
    {
        Serial.println(
            identifyMPU()
        );

        return;
    }

    // =================================================
    // AK8963
    // =================================================

    if (addr == 0x0C)
    {
        uint8_t id =
            readReg(
                addr,
                0x00
            );

        if (id == 0x48)
        {
            Serial.println(
                "AK8963 Magnetometer"
            );
        }
        else
        {
            Serial.print(
                "Unknown Magnetometer, ID=0x"
            );

            Serial.println(
                id,
                HEX
            );
        }

        return;
    }

    // =================================================
    // BMP280 / BME280 / BME680
    // =================================================

    if (addr == 0x76 ||
        addr == 0x77)
    {
        Serial.println(
            identifyBMP(addr)
        );

        return;
    }

    // =================================================
    // BNO055
    // =================================================

    if (addr == 0x28 ||
        addr == 0x29)
    {
        uint8_t id =
            readReg(
                addr,
                0x00
            );

        if (id == 0xA0)
        {
            Serial.println(
                "BNO055"
            );
        }
        else
        {
            Serial.println(
                "Possible BNO055"
            );
        }

        return;
    }

    // =================================================
    // BMI160
    // =================================================

    if (addr == 0x68 ||
        addr == 0x69)
    {
        uint8_t id =
            readReg(
                addr,
                0x00
            );

        if (id == 0xD1)
        {
            Serial.println(
                "BMI160"
            );
        }
        else
        {
            Serial.println(
                "Possible IMU"
            );
        }

        return;
    }

    // =================================================
    // HMC5883L
    // =================================================

    if (addr == 0x1E)
    {
        uint8_t a =
            readReg(
                addr,
                0x0A
            );

        uint8_t b =
            readReg(
                addr,
                0x0B
            );

        uint8_t c =
            readReg(
                addr,
                0x0C
            );

        if (a == 'H' &&
            b == '4' &&
            c == '3')
        {
            Serial.println(
                "HMC5883L Magnetometer"
            );
        }
        else
        {
            Serial.println(
                "Possible HMC5883L"
            );
        }

        return;
    }

    // =================================================
    // QMC5883L
    // =================================================

    if (addr == 0x0D)
    {
        uint8_t id =
            readReg(
                addr,
                0x0D
            );

        if (id == 0xFF)
        {
            Serial.println(
                "QMC5883L Magnetometer"
            );
        }
        else
        {
            Serial.println(
                "Possible QMC5883L"
            );
        }

        return;
    }

    // =================================================
    // LIS3DH
    // =================================================

    if (addr == 0x18 ||
        addr == 0x19)
    {
        uint8_t id =
            readReg(
                addr,
                0x0F
            );

        if (id == 0x33)
        {
            Serial.println(
                "LIS3DH"
            );
        }
        else
        {
            Serial.println(
                "Possible LIS3DH"
            );
        }

        return;
    }

    // =================================================
    // LSM6DS3
    // =================================================

    if (addr == 0x6A ||
        addr == 0x6B)
    {
        uint8_t id =
            readReg(
                addr,
                0x0F
            );

        if (id == 0x69)
        {
            Serial.println(
                "LSM6DS3"
            );
        }
        else
        {
            Serial.println(
                "Possible LSM6DS3"
            );
        }

        return;
    }

    // =================================================
    // VL53L0X
    // =================================================

    if (addr == 0x29)
    {
        Serial.println(
            "VL53L0X / VL53L1X ToF"
        );

        return;
    }

    // =================================================
    // SHT30
    // =================================================

    if (addr == 0x44 ||
        addr == 0x45)
    {
        Serial.println(
            "SHT30/SHT31/SHT35"
        );

        return;
    }

    // =================================================
    // AHT10 / AHT20
    // =================================================

    if (addr == 0x38)
    {
        Serial.println(
            "AHT10/AHT20"
        );

        return;
    }

    // =================================================
    // INA219
    // =================================================

    if (
        addr >= 0x40 &&
        addr <= 0x4F
    )
    {
        Serial.println(
            "Possible INA219/INA226"
        );

        return;
    }

    // =================================================
    // ADS1115
    // =================================================

    if (
        addr >= 0x48 &&
        addr <= 0x4B
    )
    {
        Serial.println(
            "Possible ADS1115"
        );

        return;
    }

    // =================================================
    // TCA9548A
    // =================================================

    if (
        addr >= 0x70 &&
        addr <= 0x77
    )
    {
        Serial.println(
            "Possible TCA9548A I2C Multiplexer"
        );

        return;
    }

    // =================================================
    // SSD1306 OLED
    // =================================================

    if (
        addr == 0x3C ||
        addr == 0x3D
    )
    {
        Serial.println(
            "Possible SSD1306 OLED"
        );

        return;
    }

    // =================================================
    // PCF8574
    // =================================================

    if (
        addr >= 0x20 &&
        addr <= 0x27
    )
    {
        Serial.println(
            "Possible PCF8574 I/O Expander"
        );

        return;
    }

    // =================================================
    // UNKNOWN
    // =================================================

    Serial.println(
        "Unknown I2C Device"
    );
}

// =====================================================
// SCAN
// =====================================================

void scanI2C()
{
    int count = 0;

    Serial.println();
    Serial.println(
        "=========================================="
    );

    Serial.println(
        "          I2C DEVICE SCANNER"
    );

    Serial.println(
        "=========================================="
    );

    for (
        uint8_t addr = 1;
        addr < 127;
        addr++
    )
    {
        if (exists(addr))
        {
            identifyDevice(addr);

            count++;
        }
    }

    Serial.println();
    Serial.print(
        "Tong so device: "
    );

    Serial.println(count);

    Serial.println(
        "=========================================="
    );
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
    Serial.begin(115200);

    delay(2000);

    Serial.println();
    Serial.println(
        "##########################################"
    );

    Serial.println(
        " ESP32-C3 SUPER MINI"
    );

    Serial.println(
        " UNIVERSAL I2C SENSOR SCANNER"
    );

    Serial.println(
        "##########################################"
    );

    Serial.print(
        "SDA = GPIO"
    );

    Serial.println(SDA_PIN);

    Serial.print(
        "SCL = GPIO"
    );

    Serial.println(SCL_PIN);

    // =================================================
    // I2C START
    // =================================================

    Wire.begin(
        SDA_PIN,
        SCL_PIN
    );

    // 100kHz ổn định hơn khi dây dài
    Wire.setClock(100000);

    delay(100);

    // =================================================
    // SCAN LẦN 1
    // =================================================

    Serial.println();
    Serial.println(
        "SCAN LAN 1"
    );

    scanI2C();

    // =================================================
    // MPU9250 BYPASS
    // =================================================

    Serial.println();
    Serial.println(
        "Kiem tra MPU9250..."
    );

    if (exists(MPU_ADDR))
    {
        uint8_t id =
            readReg(
                MPU_ADDR,
                0x75
            );

        Serial.print(
            "MPU WHO_AM_I = 0x"
        );

        Serial.println(
            id,
            HEX
        );

        if (id == 0x71)
        {
            Serial.println(
                "Detected MPU9250"
            );

            Serial.println(
                "Bat I2C Bypass..."
            );

            enableMPUBypass();

            Serial.println(
                "Bypass complete"
            );
        }
        else if (id == 0x70)
        {
            Serial.println(
                "Detected MPU6500"
            );
        }
        else if (id == 0x68)
        {
            Serial.println(
                "Detected MPU6050"
            );
        }
        else if (id == 0x73)
        {
            Serial.println(
                "Detected MPU9255"
            );

            enableMPUBypass();
        }
    }

    // =================================================
    // SCAN LAN 2
    // =================================================

    Serial.println();
    Serial.println(
        "SCAN LAN 2 - SAU KHI BYPASS"
    );

    scanI2C();

    // =================================================
    // AK8963 TEST
    // =================================================

    Serial.println();

    if (checkAK8963())
    {
        Serial.println(
            "**************************************"
        );

        Serial.println(
            " AK8963 FOUND"
        );

        Serial.println(
            " Magnetometer OK"
        );

        Serial.println(
            "**************************************"
        );
    }
    else
    {
        Serial.println(
            "AK8963 not found"
        );
    }

    Serial.println();
    Serial.println(
        "SCAN HOAN TAT"
    );
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
    // Không làm gì
}


// #include <Arduino.h>
// #include <Wire.h>

// // =====================================================
// // PIN
// // =====================================================

// #define SDA_PIN 8
// #define SCL_PIN 9

// // =====================================================
// // I2C ADDRESS
// // =====================================================

// #define MPU_ADDR 0x68
// #define AK_ADDR  0x0C
// #define BMP_ADDR 0x76

// // =====================================================
// // MPU REGISTERS
// // =====================================================

// #define MPU_WHO_AM_I    0x75
// #define MPU_PWR_MGMT_1  0x6B
// #define MPU_INT_PIN_CFG 0x37

// // =====================================================
// // AK8963 REGISTERS
// // =====================================================

// #define AK_WHO_AM_I 0x00

// // =====================================================
// // READ REGISTER
// // =====================================================

// uint8_t readRegister(
//     uint8_t address,
//     uint8_t reg
// )
// {
//     Wire.beginTransmission(address);

//     Wire.write(reg);

//     uint8_t error =
//         Wire.endTransmission(false);

//     if (error != 0)
//     {
//         return 0xFF;
//     }

//     uint8_t received =
//         Wire.requestFrom(
//             address,
//             (uint8_t)1
//         );

//     if (received != 1)
//     {
//         return 0xFF;
//     }

//     if (Wire.available())
//     {
//         return Wire.read();
//     }

//     return 0xFF;
// }

// // =====================================================
// // WRITE REGISTER
// // =====================================================

// bool writeRegister(
//     uint8_t address,
//     uint8_t reg,
//     uint8_t value
// )
// {
//     Wire.beginTransmission(address);

//     Wire.write(reg);
//     Wire.write(value);

//     uint8_t error =
//         Wire.endTransmission();

//     return error == 0;
// }

// // =====================================================
// // CHECK DEVICE
// // =====================================================

// bool deviceExists(
//     uint8_t address
// )
// {
//     Wire.beginTransmission(address);

//     uint8_t error =
//         Wire.endTransmission();

//     return error == 0;
// }

// // =====================================================
// // IDENTIFY MPU
// // =====================================================

// const char* identifyMPU(
//     uint8_t who
// )
// {
//     switch (who)
//     {
//         case 0x68:
//             return "MPU6050";

//         case 0x70:
//             return "MPU6500";

//         case 0x71:
//             return "MPU9250";

//         case 0x73:
//             return "MPU9255";

//         default:
//             return "UNKNOWN";
//     }
// }

// // =====================================================
// // WAKE MPU
// // =====================================================

// bool wakeMPU()
// {
//     return writeRegister(
//         MPU_ADDR,
//         MPU_PWR_MGMT_1,
//         0x00
//     );
// }

// // =====================================================
// // ENABLE I2C BYPASS
// // =====================================================

// bool enableBypass()
// {
//     return writeRegister(
//         MPU_ADDR,
//         MPU_INT_PIN_CFG,
//         0x02
//     );
// }

// // =====================================================
// // SCAN 1 LẦN
// // =====================================================

// void scanOnce()
// {
//     Serial.println();
//     Serial.println(
//         "--------------------------------"
//     );

//     Serial.println(
//         "I2C SCAN"
//     );

//     Serial.println(
//         "--------------------------------"
//     );

//     for (
//         uint8_t address = 1;
//         address < 127;
//         address++
//     )
//     {
//         Wire.beginTransmission(
//             address
//         );

//         if (
//             Wire.endTransmission() == 0
//         )
//         {
//             Serial.print("0x");

//             if (address < 0x10)
//                 Serial.print("0");

//             Serial.print(
//                 address,
//                 HEX
//             );

//             if (address == MPU_ADDR)
//             {
//                 Serial.print(
//                     " -> "
//                 );

//                 uint8_t who =
//                     readRegister(
//                         MPU_ADDR,
//                         MPU_WHO_AM_I
//                     );

//                 Serial.print(
//                     identifyMPU(who)
//                 );
//             }

//             else if (address == AK_ADDR)
//             {
//                 Serial.print(
//                     " -> AK8963"
//                 );
//             }

//             else if (address == BMP_ADDR)
//             {
//                 uint8_t chipID =
//                     readRegister(
//                         BMP_ADDR,
//                         0xD0
//                     );

//                 if (chipID == 0x58)
//                 {
//                     Serial.print(
//                         " -> BMP280"
//                     );
//                 }
//                 else if (chipID == 0x60)
//                 {
//                     Serial.print(
//                         " -> BME280"
//                     );
//                 }
//                 else
//                 {
//                     Serial.print(
//                         " -> BMP/BME UNKNOWN"
//                     );
//                 }
//             }

//             else
//             {
//                 Serial.print(
//                     " -> UNKNOWN I2C"
//                 );
//             }

//             Serial.println();
//         }
//     }
// }

// // =====================================================
// // TEST AK8963
// // =====================================================

// bool testAK8963()
// {
//     if (!deviceExists(AK_ADDR))
//     {
//         return false;
//     }

//     uint8_t who =
//         readRegister(
//             AK_ADDR,
//             AK_WHO_AM_I
//         );

//     return who == 0x48;
// }

// // =====================================================
// // MAIN
// // =====================================================

// void setup()
// {
//     Serial.begin(115200);

//     delay(2000);

//     Serial.println();
//     Serial.println(
//         "=========================================="
//     );

//     Serial.println(
//         " ESP32-C3 MPU / AK8963 / BMP280 TEST"
//     );

//     Serial.println(
//         "=========================================="
//     );

//     Serial.print(
//         "SDA = GPIO"
//     );

//     Serial.println(SDA_PIN);

//     Serial.print(
//         "SCL = GPIO"
//     );

//     Serial.println(SCL_PIN);

//     // =================================================
//     // I2C
//     // =================================================

//     Wire.begin(
//         SDA_PIN,
//         SCL_PIN
//     );

//     // Dùng 100kHz để test ổn định
//     Wire.setClock(100000);

//     delay(100);

//     // =================================================
//     // MPU CHECK
//     // =================================================

//     Serial.println();
//     Serial.println(
//         "========== MPU CHECK =========="
//     );

//     if (!deviceExists(MPU_ADDR))
//     {
//         Serial.println(
//             "Khong tim thay device 0x68!"
//         );

//         return;
//     }

//     uint8_t mpuWho =
//         readRegister(
//             MPU_ADDR,
//             MPU_WHO_AM_I
//         );

//     Serial.print(
//         "WHO_AM_I = 0x"
//     );

//     if (mpuWho < 0x10)
//         Serial.print("0");

//     Serial.println(
//         mpuWho,
//         HEX
//     );

//     Serial.print(
//         "Detected: "
//     );

//     Serial.println(
//         identifyMPU(mpuWho)
//     );

//     // =================================================
//     // WAKE MPU
//     // =================================================

//     Serial.println();

//     Serial.println(
//         "Wake MPU..."
//     );

//     if (wakeMPU())
//     {
//         Serial.println(
//             "Wake OK"
//         );
//     }
//     else
//     {
//         Serial.println(
//             "Wake ERROR"
//         );
//     }

//     delay(100);

//     // =================================================
//     // BYPASS
//     // =================================================

//     Serial.println();

//     Serial.println(
//         "Enable I2C Bypass..."
//     );

//     if (enableBypass())
//     {
//         Serial.println(
//             "Bypass OK"
//         );
//     }
//     else
//     {
//         Serial.println(
//             "Bypass ERROR"
//         );
//     }

//     delay(100);

//     // =================================================
//     // INITIAL SCAN
//     // =================================================

//     Serial.println();
//     Serial.println(
//         "========== INITIAL SCAN =========="
//     );

//     scanOnce();

//     // =================================================
//     // TEST 100 TIMES
//     // =================================================

//     Serial.println();
//     Serial.println(
//         "=========================================="
//     );

//     Serial.println(
//         " BAT DAU TEST AK8963 100 LAN"
//     );

//     Serial.println(
//         " I2C = 100 kHz"
//     );

//     Serial.println(
//         "=========================================="
//     );

//     int akOK = 0;
//     int akFail = 0;

//     for (int i = 1; i <= 100; i++)
//     {
//         bool result =
//             testAK8963();

//         if (result)
//         {
//             akOK++;

//             Serial.print(
//                 "["
//             );

//             Serial.print(i);

//             Serial.println(
//                 "] AK8963 = OK"
//             );
//         }
//         else
//         {
//             akFail++;

//             Serial.print(
//                 "["
//             );

//             Serial.print(i);

//             Serial.println(
//                 "] AK8963 = FAIL"
//             );
//         }

//         delay(100);
//     }

//     // =================================================
//     // RESULT
//     // =================================================

//     Serial.println();
//     Serial.println(
//         "=========================================="
//     );

//     Serial.println(
//         "             KET QUA"
//     );

//     Serial.println(
//         "=========================================="

//     );

//     Serial.print(
//         "AK8963 OK   : "
//     );

//     Serial.println(
//         akOK
//     );

//     Serial.print(
//         "AK8963 FAIL : "
//     );

//     Serial.println(
//         akFail
//     );

//     Serial.println();

//     if (akOK == 100)
//     {
//         Serial.println(
//             ">>> AK8963 ON DINH 100/100"
//         );
//     }
//     else if (akOK > 0)
//     {
//         Serial.println(
//             ">>> AK8963 LUC CO LUC MAT!"
//         );

//         Serial.println(
//             "Can kiem tra day SDA/SCL/VCC/GND."
//         );

//         Serial.println(
//             "Dong thoi thu tiep I2C 50kHz."
//         );
//     }
//     else
//     {
//         Serial.println(
//             ">>> KHONG DOC DUOC AK8963"
//         );
//     }

//     // =================================================
//     // FINAL SCAN
//     // =================================================

//     Serial.println();
//     Serial.println(
//         "========== FINAL SCAN =========="
//     );

//     scanOnce();

//     Serial.println();
//     Serial.println(
//         "=========================================="
//     );

//     Serial.println(
//         " TEST HOAN TAT"
//     );

//     Serial.println(
//         "=========================================="
//     );
// }

// void loop()
// {
// }




// #include <Arduino.h>
// #include <Wire.h>

// #define SDA_PIN 8
// #define SCL_PIN 9

// #define MPU_ADDR 0x68
// #define AK_ADDR  0x0C

// #define WHO_AM_I     0x75
// #define PWR_MGMT_1   0x6B
// #define INT_PIN_CFG  0x37

// // =====================================================
// // READ REGISTER
// // =====================================================

// uint8_t readRegister(uint8_t address, uint8_t reg)
// {
//     Wire.beginTransmission(address);
//     Wire.write(reg);

//     if (Wire.endTransmission(false) != 0)
//         return 0xFF;

//     Wire.requestFrom(address, (uint8_t)1);

//     if (Wire.available())
//         return Wire.read();

//     return 0xFF;
// }

// // =====================================================
// // WRITE REGISTER
// // =====================================================

// bool writeRegister(
//     uint8_t address,
//     uint8_t reg,
//     uint8_t value
// )
// {
//     Wire.beginTransmission(address);

//     Wire.write(reg);
//     Wire.write(value);

//     return Wire.endTransmission() == 0;
// }

// // =====================================================
// // CHECK DEVICE
// // =====================================================

// bool deviceExists(uint8_t address)
// {
//     Wire.beginTransmission(address);

//     return Wire.endTransmission() == 0;
// }

// // =====================================================
// // NHẬN DẠNG 0x68
// // =====================================================

// const char* identifyMPU()
// {
//     uint8_t who =
//         readRegister(
//             MPU_ADDR,
//             WHO_AM_I
//         );

//     if (who == 0x68)
//         return "MPU6050";

//     if (who == 0x71)
//         return "MPU9250";

//     if (who == 0x73)
//         return "MPU9255";

//     return "Unknown MPU";
// }

// // =====================================================
// // BẬT BYPASS CHO MPU9250
// // =====================================================

// bool enableMPU9250Bypass()
// {
//     uint8_t who =
//         readRegister(
//             MPU_ADDR,
//             WHO_AM_I
//         );

//     if (who != 0x71)
//         return false;

//     // Wake up
//     writeRegister(
//         MPU_ADDR,
//         PWR_MGMT_1,
//         0x00
//     );

//     delay(100);

//     // I2C Bypass
//     bool result =
//         writeRegister(
//             MPU_ADDR,
//             INT_PIN_CFG,
//             0x02
//         );

//     delay(20);

//     return result;
// }

// // =====================================================
// // IN TÊN THIẾT BỊ THEO ADDRESS
// // =====================================================

// void printDeviceName(uint8_t address)
// {
//     // -------------------------
//     // MPU
//     // -------------------------

//     if (address == 0x68)
//     {
//         uint8_t who =
//             readRegister(
//                 0x68,
//                 WHO_AM_I
//             );

//         Serial.print(" -> ");

//         if (who == 0x68)
//             Serial.print("MPU6050");

//         else if (who == 0x71)
//             Serial.print("MPU9250");

//         else if (who == 0x73)
//             Serial.print("MPU9255");

//         else
//             Serial.print("Unknown MPU");

//         return;
//     }

//     // -------------------------
//     // AK8963
//     // -------------------------

//     if (address == 0x0C)
//     {
//         uint8_t who =
//             readRegister(
//                 0x0C,
//                 0x00
//             );

//         Serial.print(" -> ");

//         if (who == 0x48)
//             Serial.print(
//                 "AK8963 Magnetometer"
//             );
//         else
//             Serial.print(
//                 "Unknown Magnetometer"
//             );

//         return;
//     }

//     // -------------------------
//     // BMP280 / BME280
//     // -------------------------

//     if (
//         address == 0x76 ||
//         address == 0x77
//     )
//     {
//         uint8_t id =
//             readRegister(
//                 address,
//                 0xD0
//             );

//         Serial.print(" -> ");

//         if (id == 0x58)
//             Serial.print("BMP280");

//         else if (id == 0x60)
//             Serial.print("BME280");

//         else
//             Serial.print(
//                 "BMP/BME280 Unknown"
//             );

//         return;
//     }

//     // -------------------------
//     // OLED
//     // -------------------------

//     if (
//         address == 0x3C ||
//         address == 0x3D
//     )
//     {
//         Serial.print(
//             " -> OLED SSD1306"
//         );

//         return;
//     }

//     // -------------------------
//     // LCD
//     // -------------------------

//     if (address == 0x27)
//     {
//         Serial.print(
//             " -> LCD I2C PCF8574"
//         );

//         return;
//     }

//     if (address == 0x20)
//     {
//         Serial.print(
//             " -> PCF8574 I/O Expander"
//         );

//         return;
//     }

//     // -------------------------
//     // UNKNOWN
//     // -------------------------

//     Serial.print(
//         " -> Unknown I2C Device"
//     );
// }

// // =====================================================
// // SCAN I2C
// // =====================================================

// int scanI2C()
// {
//     Serial.println();
//     Serial.println(
//         "======================================"
//     );

//     Serial.println(
//         "          I2C SENSOR SCANNER"
//     );

//     Serial.println(
//         "======================================"
//     );

//     int count = 0;

//     for (
//         uint8_t address = 1;
//         address < 127;
//         address++
//     )
//     {
//         Wire.beginTransmission(address);

//         uint8_t error =
//             Wire.endTransmission();

//         if (error == 0)
//         {
//             Serial.print("0x");

//             if (address < 0x10)
//                 Serial.print("0");

//             Serial.print(
//                 address,
//                 HEX
//             );

//             printDeviceName(address);

//             Serial.println();

//             count++;
//         }
//     }

//     Serial.println();
//     Serial.print(
//         "Tong so I2C device: "
//     );
//     Serial.println(count);

//     Serial.println(
//         "======================================"
//     );

//     return count;
// }

// // =====================================================
// // SETUP
// // =====================================================

// void setup()
// {
//     Serial.begin(115200);

//     delay(2000);

//     Serial.println();
//     Serial.println(
//         "######################################"
//     );

//     Serial.println(
//         " ESP32-C3 SUPER MINI"
//     );

//     Serial.println(
//         " SENSOR AUTO DETECTION"
//     );

//     Serial.println(
//         "######################################"
//     );

//     Serial.print(
//         "SDA = GPIO"
//     );
//     Serial.println(SDA_PIN);

//     Serial.print(
//         "SCL = GPIO"
//     );
//     Serial.println(SCL_PIN);

//     // I2C
//     Wire.begin(
//         SDA_PIN,
//         SCL_PIN
//     );

//     Wire.setClock(400000);

//     delay(100);

//     // =========================================
//     // SCAN LẦN 1
//     // =========================================

//     Serial.println();
//     Serial.println(
//         "SCAN I2C LAN 1:"
//     );

//     scanI2C();

//     // =========================================
//     // KIỂM TRA MPU9250
//     // =========================================

//     if (deviceExists(MPU_ADDR))
//     {
//         uint8_t who =
//             readRegister(
//                 MPU_ADDR,
//                 WHO_AM_I
//             );

//         Serial.println();
//         Serial.println(
//             "MPU DETECTION:"
//         );

//         Serial.print(
//             "WHO_AM_I = 0x"
//         );

//         Serial.println(
//             who,
//             HEX
//         );

//         if (who == 0x71)
//         {
//             Serial.println(
//                 "Detected: MPU9250"
//             );

//             Serial.println(
//                 "Enable I2C Bypass..."
//             );

//             if (enableMPU9250Bypass())
//             {
//                 Serial.println(
//                     "Bypass OK"
//                 );
//             }
//             else
//             {
//                 Serial.println(
//                     "Bypass ERROR"
//                 );
//             }
//         }

//         else if (who == 0x68)
//         {
//             Serial.println(
//                 "Detected: MPU6050"
//             );
//         }

//         else if (who == 0x73)
//         {
//             Serial.println(
//                 "Detected: MPU9255"
//             );
//         }

//         else
//         {
//             Serial.println(
//                 "Unknown MPU"
//             );
//         }
//     }

//     // =========================================
//     // SCAN LẦN 2
//     // =========================================

//     Serial.println();
//     Serial.println(
//         "SCAN I2C LAN 2:"
//     );

//     scanI2C();

//     // =========================================
//     // KIỂM TRA AK8963
//     // =========================================

//     if (deviceExists(AK_ADDR))
//     {
//         uint8_t who =
//             readRegister(
//                 AK_ADDR,
//                 0x00
//             );

//         Serial.println();
//         Serial.println(
//             "MAGNETOMETER:"
//         );

//         Serial.print(
//             "0x0C WHO_AM_I = 0x"
//         );

//         Serial.println(
//             who,
//             HEX
//         );

//         if (who == 0x48)
//         {
//             Serial.println(
//                 "Detected: AK8963 Magnetometer"
//             );
//         }
//         else
//         {
//             Serial.println(
//                 "Unknown magnetometer"
//             );
//         }
//     }

//     Serial.println();
//     Serial.println(
//         "######################################"
//     );

//     Serial.println(
//         "SCAN HOAN TAT"
//     );

//     Serial.println(
//         "######################################"
//     );
// }

// void loop()
// {
// }




// // #include <Arduino.h>
// // #include <Wire.h>

// // #define SDA_PIN 8
// // #define SCL_PIN 9

// // #define MPU_ADDR 0x68

// // #define PWR_MGMT_1  0x6B
// // #define INT_PIN_CFG 0x37
// // #define WHO_AM_I    0x75

// // // =====================================================
// // // GHI REGISTER
// // // =====================================================

// // bool writeRegister(
// //     uint8_t address,
// //     uint8_t reg,
// //     uint8_t value
// // )
// // {
// //     Wire.beginTransmission(address);

// //     Wire.write(reg);
// //     Wire.write(value);

// //     uint8_t error = Wire.endTransmission();

// //     if (error != 0)
// //     {
// //         return false;
// //     }

// //     return true;
// // }

// // // =====================================================
// // // ĐỌC REGISTER
// // // =====================================================

// // uint8_t readRegister(
// //     uint8_t address,
// //     uint8_t reg
// // )
// // {
// //     Wire.beginTransmission(address);

// //     Wire.write(reg);

// //     if (Wire.endTransmission(false) != 0)
// //     {
// //         return 0xFF;
// //     }

// //     Wire.requestFrom(
// //         address,
// //         (uint8_t)1
// //     );

// //     if (Wire.available())
// //     {
// //         return Wire.read();
// //     }

// //     return 0xFF;
// // }

// // // =====================================================
// // // BẬT MPU9250
// // // =====================================================

// // bool setupMPU9250()
// // {
// //     Serial.println();
// //     Serial.println("Khoi dong MPU9250...");

// //     // Kiểm tra MPU9250
// //     uint8_t who =
// //         readRegister(
// //             MPU_ADDR,
// //             WHO_AM_I
// //         );

// //     Serial.print("MPU WHO_AM_I = 0x");
// //     Serial.println(who, HEX);

// //     if (who == 0xFF)
// //     {
// //         Serial.println(
// //             "Khong ket noi duoc MPU9250!"
// //         );

// //         return false;
// //     }

// //     if (who == 0x71)
// //     {
// //         Serial.println(
// //             "MPU9250 detected"
// //         );
// //     }
// //     else
// //     {
// //         Serial.println(
// //             "Khong phai MPU9250 0x71"
// //         );
// //     }

// //     // Đánh thức MPU9250
// //     if (!writeRegister(
// //             MPU_ADDR,
// //             PWR_MGMT_1,
// //             0x00))
// //     {
// //         Serial.println(
// //             "Loi PWR_MGMT_1"
// //         );

// //         return false;
// //     }

// //     delay(100);

// //     // =================================================
// //     // QUAN TRỌNG:
// //     // Bật I2C BYPASS
// //     // =================================================

// //     if (!writeRegister(
// //             MPU_ADDR,
// //             INT_PIN_CFG,
// //             0x02))
// //     {
// //         Serial.println(
// //             "Loi bat I2C BYPASS"
// //         );

// //         return false;
// //     }

// //     delay(20);

// //     Serial.println(
// //         "MPU9250 I2C BYPASS = ON"
// //     );

// //     return true;
// // }

// // // =====================================================
// // // SCAN I2C
// // // =====================================================

// // void scanI2C()
// // {
// //     Serial.println();
// //     Serial.println(
// //         "================================"
// //     );

// //     Serial.println(
// //         "        I2C SCANNER"
// //     );

// //     Serial.println(
// //         "================================"
// //     );

// //     int count = 0;

// //     for (
// //         uint8_t address = 1;
// //         address < 127;
// //         address++
// //     )
// //     {
// //         Wire.beginTransmission(address);

// //         uint8_t error =
// //             Wire.endTransmission();

// //         if (error == 0)
// //         {
// //             Serial.print(
// //                 "FOUND: 0x"
// //             );

// //             if (address < 0x10)
// //             {
// //                 Serial.print("0");
// //             }

// //             Serial.print(
// //                 address,
// //                 HEX
// //             );

// //             Serial.print("  ");

// //             // Nhận dạng
// //             if (address == 0x68)
// //             {
// //                 Serial.println(
// //                     "MPU9250"
// //                 );
// //             }
// //             else if (address == 0x0C)
// //             {
// //                 Serial.println(
// //                     "AK8963 Magnetometer"
// //                 );
// //             }
// //             else if (address == 0x76)
// //             {
// //                 Serial.println(
// //                     "BMP280 / BME280"
// //                 );
// //             }
// //             else if (address == 0x77)
// //             {
// //                 Serial.println(
// //                     "BMP280 / BME280"
// //                 );
// //             }
// //             else if (address == 0x3C)
// //             {
// //                 Serial.println(
// //                     "OLED SSD1306"
// //                 );
// //             }
// //             else if (address == 0x3D)
// //             {
// //                 Serial.println(
// //                     "OLED SSD1306"
// //                 );
// //             }
// //             else if (address == 0x27)
// //             {
// //                 Serial.println(
// //                     "LCD PCF8574"
// //                 );
// //             }
// //             else
// //             {
// //                 Serial.println(
// //                     "UNKNOWN DEVICE"
// //                 );
// //             }

// //             count++;
// //         }
// //     }

// //     Serial.println();

// //     Serial.print(
// //         "Tong so I2C device: "
// //     );

// //     Serial.println(count);

// //     Serial.println(
// //         "================================"
// //     );
// // }

// // // =====================================================
// // // SETUP
// // // =====================================================

// // void setup()
// // {
// //     Serial.begin(115200);

// //     delay(1500);

// //     Serial.println();
// //     Serial.println(
// //         "================================"
// //     );

// //     Serial.println(
// //         " ESP32-C3 SUPER MINI"
// //     );

// //     Serial.println(
// //         " I2C SENSOR SCANNER"
// //     );

// //     Serial.println(
// //         "================================"
// //     );

// //     Serial.print(
// //         "SDA = GPIO"
// //     );

// //     Serial.println(SDA_PIN);

// //     Serial.print(
// //         "SCL = GPIO"
// //     );

// //     Serial.println(SCL_PIN);

// //     // Khởi tạo I2C
// //     Wire.begin(
// //         SDA_PIN,
// //         SCL_PIN
// //     );

// //     Wire.setClock(400000);

// //     delay(100);

// //     // =================================================
// //     // BƯỚC 1:
// //     // Kiểm tra MPU9250 và bật bypass
// //     // =================================================

// //     setupMPU9250();

// //     // =================================================
// //     // BƯỚC 2:
// //     // Scan tất cả thiết bị
// //     // =================================================

// //     scanI2C();
// // }

// // // =====================================================
// // // LOOP
// // // =====================================================

// // void loop()
// // {
// //     // Không cần scan liên tục
// // }














// // //  #include <Arduino.h>

// // // #include <Wire.h>

// // // #define SDA_PIN 8
// // // #define SCL_PIN 9

// // // #define MPU_ADDR 0x68
// // // #define MAG_ADDR 0x0C

// // // // MPU9250 registers
// // // #define WHO_AM_I_MPU 0x75
// // // #define PWR_MGMT_1   0x6B
// // // #define INT_PIN_CFG  0x37
// // // #define ACCEL_XOUT_H 0x3B

// // // // AK8963 registers
// // // #define WHO_AM_I_AK  0x00
// // // #define ST1         0x02
// // // #define HXL         0x03
// // // #define CNTL1       0x0A

// // // void writeRegister(uint8_t addr, uint8_t reg, uint8_t value)
// // // {
// // //     Wire.beginTransmission(addr);
// // //     Wire.write(reg);
// // //     Wire.write(value);

// // //     if (Wire.endTransmission() != 0) {
// // //         Serial.printf("Loi ghi I2C: 0x%02X\n", addr);
// // //     }
// // // }

// // // uint8_t readRegister(uint8_t addr, uint8_t reg)
// // // {
// // //     Wire.beginTransmission(addr);
// // //     Wire.write(reg);

// // //     if (Wire.endTransmission(false) != 0) {
// // //         return 0xFF;
// // //     }

// // //     Wire.requestFrom(addr, (uint8_t)1);

// // //     if (Wire.available()) {
// // //         return Wire.read();
// // //     }

// // //     return 0xFF;
// // // }

// // // bool readBytes(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len)
// // // {
// // //     Wire.beginTransmission(addr);
// // //     Wire.write(reg);

// // //     if (Wire.endTransmission(false) != 0) {
// // //         return false;
// // //     }

// // //     uint8_t received = Wire.requestFrom(addr, len);

// // //     if (received != len) {
// // //         return false;
// // //     }

// // //     for (uint8_t i = 0; i < len; i++) {
// // //         data[i] = Wire.read();
// // //     }

// // //     return true;
// // // }

// // // void setupMPU9250()
// // // {
// // //     // Wake up MPU9250
// // //     writeRegister(MPU_ADDR, PWR_MGMT_1, 0x00);

// // //     delay(100);

// // //     // Enable I2C bypass để ESP32 truy cập trực tiếp AK8963
// // //     writeRegister(MPU_ADDR, INT_PIN_CFG, 0x02);

// // //     delay(10);
// // // }

// // // bool setupAK8963()
// // // {
// // //     uint8_t who = readRegister(MAG_ADDR, WHO_AM_I_AK);

// // //     Serial.print("AK8963 WHO_AM_I = 0x");
// // //     Serial.println(who, HEX);

// // //     if (who != 0x48) {
// // //         return false;
// // //     }

// // //     // Power down
// // //     writeRegister(MAG_ADDR, CNTL1, 0x00);
// // //     delay(10);

// // //     // Continuous measurement mode 2
// // //     // 100Hz, 16-bit
// // //     writeRegister(MAG_ADDR, CNTL1, 0x16);
// // //     delay(10);

// // //     return true;
// // // }

// // // void readMPU()
// // // {
// // //     uint8_t data[14];

// // //     if (!readBytes(MPU_ADDR, ACCEL_XOUT_H, data, 14)) {
// // //         Serial.println("Khong doc duoc MPU9250!");
// // //         return;
// // //     }

// // //     int16_t ax = (int16_t)((data[0] << 8) | data[1]);
// // //     int16_t ay = (int16_t)((data[2] << 8) | data[3]);
// // //     int16_t az = (int16_t)((data[4] << 8) | data[5]);

// // //     int16_t temp = (int16_t)((data[6] << 8) | data[7]);

// // //     int16_t gx = (int16_t)((data[8] << 8) | data[9]);
// // //     int16_t gy = (int16_t)((data[10] << 8) | data[11]);
// // //     int16_t gz = (int16_t)((data[12] << 8) | data[13]);

// // //     // Default MPU9250:
// // //     // Accelerometer ±2g
// // //     // Gyroscope ±250 DPS

// // //     float ax_g = ax / 16384.0;
// // //     float ay_g = ay / 16384.0;
// // //     float az_g = az / 16384.0;

// // //     float gx_dps = gx / 131.0;
// // //     float gy_dps = gy / 131.0;
// // //     float gz_dps = gz / 131.0;

// // //     Serial.print("ACC: ");
// // //     Serial.print(ax_g, 3);
// // //     Serial.print("  ");
// // //     Serial.print(ay_g, 3);
// // //     Serial.print("  ");
// // //     Serial.print(az_g, 3);

// // //     Serial.print("   GYRO: ");
// // //     Serial.print(gx_dps, 2);
// // //     Serial.print("  ");
// // //     Serial.print(gy_dps, 2);
// // //     Serial.print("  ");
// // //     Serial.print(gz_dps, 2);

// // //     Serial.print("   TEMP: ");
// // //     Serial.println(temp / 333.87 + 21.0);
// // // }

// // // void readMagnetometer()
// // // {
// // //     uint8_t st1 = readRegister(MAG_ADDR, ST1);

// // //     // DRDY = 1
// // //     if (!(st1 & 0x01)) {
// // //         return;
// // //     }

// // //     uint8_t data[7];

// // //     if (!readBytes(MAG_ADDR, HXL, data, 7)) {
// // //         Serial.println("Khong doc duoc AK8963!");
// // //         return;
// // //     }

// // //     int16_t mx = (int16_t)((data[1] << 8) | data[0]);
// // //     int16_t my = (int16_t)((data[3] << 8) | data[2]);
// // //     int16_t mz = (int16_t)((data[5] << 8) | data[4]);

// // //     uint8_t st2 = data[6];

// // //     // Magnetic overflow
// // //     if (st2 & 0x08) {
// // //         Serial.println("AK8963 MAG OVERFLOW!");
// // //         return;
// // //     }

// // //     Serial.print("MAG: ");
// // //     Serial.print(mx);
// // //     Serial.print("  ");
// // //     Serial.print(my);
// // //     Serial.print("  ");
// // //     Serial.println(mz);
// // // }

// // // void setup()
// // // {
// // //     Serial.begin(115200);
// // //     delay(1000);

// // //     Serial.println();
// // //     Serial.println("==============================");
// // //     Serial.println(" ESP32-C3 + MPU9250 TEST");
// // //     Serial.println("==============================");

// // //     Wire.begin(SDA_PIN, SCL_PIN);
// // //     Wire.setClock(400000);

// // //     // -------------------------
// // //     // MPU9250
// // //     // -------------------------

// // //     uint8_t mpuWho = readRegister(MPU_ADDR, WHO_AM_I_MPU);

// // //     Serial.print("MPU9250 WHO_AM_I = 0x");
// // //     Serial.println(mpuWho, HEX);

// // //     if (mpuWho == 0x71) {
// // //         Serial.println("OK - MPU9250 detected!");
// // //     }
// // //     else if (mpuWho == 0x73) {
// // //         Serial.println("MPU9255 detected!");
// // //     }
// // //     else {
// // //         Serial.println("ERROR - MPU9250 not detected!");
// // //     }

// // //     setupMPU9250();

// // //     // -------------------------
// // //     // AK8963
// // //     // -------------------------

// // //     if (setupAK8963()) {
// // //         Serial.println("OK - AK8963 detected!");
// // //     }
// // //     else {
// // //         Serial.println("ERROR - AK8963 not detected!");
// // //     }

// // //     Serial.println();
// // //     Serial.println("Bat dau doc sensor...");
// // //     Serial.println();
// // // }

// // // void loop()
// // // {
// // //     readMPU();
// // //     readMagnetometer();

// // //     Serial.println("-----------------------------");

// // //     delay(100);
// // // }



// // // // #include <Arduino.h>
// // // // #include <Wire.h>
// // // // #include <WiFi.h>
// // // // #include <esp_now.h>

// // // // // Chân PWM cho từng motor
// // // // int M1 = 3;
// // // // int M2 = 6;
// // // // int M3 = 5;
// // // // int M4 = 4;


// // // // float RateRoll, RatePitch, RateYaw;
// // // // float RateCalibrationGyroRoll, RateCalibrationGyroPitch, RateCalibrationGyroYaw;


// // // // float RateCalibrationAccRoll, RateCalibrationAccPitch, RateCalibrationAccYaw;
// // // // int RateCalibrationNumber;
// // // // float ReceiverValue[]={0, 0, 0, 0, 0, 0, 0, 0};
// // // // int ChannelNumber=0; 
// // // // float Voltage, Current, BatteryRemaining, BatteryAtStart;
// // // // float CurrentConsumed=0;
// // // // float BatteryDefault=1300;
// // // // uint32_t LoopTimer;
// // // // float dt = 0.004f;
// // // // float DesiredRateRoll, DesiredRatePitch,DesiredRateYaw;
// // // // float ErrorRateRoll, ErrorRatePitch, ErrorRateYaw;
// // // // float InputRoll, InputThrottle, InputPitch, InputYaw;
// // // // float PrevErrorRateRoll, PrevErrorRatePitch, PrevErrorRateYaw;
// // // // float PrevItermRateRoll, PrevItermRatePitch, PrevItermRateYaw;
// // // // float PIDReturn[]={0, 0, 0};
// // // // // float PRateRoll= 1.14; float PRatePitch=PRateRoll; float PRateYaw=0;
// // // // // float IRateRoll=0.05; float IRatePitch=IRateRoll; float IRateYaw=0.0;
// // // // // float DRateRoll=0.03; float DRatePitch=DRateRoll; float DRateYaw=0.0;
// // // // float PRateRoll= 1.14; float PRatePitch=PRateRoll; float PRateYaw=0.3;
// // // // float IRateRoll=0.08; float IRatePitch=IRateRoll; float IRateYaw=0.05;
// // // // float DRateRoll=0.02; float DRatePitch=DRateRoll; float DRateYaw=0.01;
// // // // float MotorInput1, MotorInput2, MotorInput3, MotorInput4;
// // // // float AccX, AccY, AccZ;
// // // // float AngleRoll, AnglePitch;
// // // // float KalmanAngleRoll=0, KalmanUncertaintyAngleRoll=2*2;
// // // // float KalmanAnglePitch=0, KalmanUncertaintyAnglePitch=2*2;
// // // // float Kalman1DOutput[]={0,0};
// // // // float DesiredAngleRoll, DesiredAnglePitch;
// // // // float ErrorAngleRoll, ErrorAnglePitch;
// // // // float PrevErrorAngleRoll, PrevErrorAnglePitch;
// // // // float PrevItermAngleRoll, PrevItermAnglePitch;
// // // // float PAngleRoll= 1.14; float PAnglePitch=PAngleRoll;
// // // // float IAngleRoll=0.08; float IAnglePitch=IAngleRoll;
// // // // float DAngleRoll=0.02; float DAnglePitch=DAngleRoll;

// // // // int16_t AccXLSB;
// // // // int16_t AccYLSB;
// // // // int16_t AccZLSB;
// // // // int16_t GyroX;
// // // // int16_t GyroY;
// // // // int16_t GyroZ;

// // // // float AccXFilter = 0;
// // // // float AccYFilter = 0;
// // // // float AccZFilter = 1;

// // // // float GyroRollFilter  = 0;
// // // // float GyroPitchFilter = 0;
// // // // float GyroYawFilter   = 0;

// // // // void kalman_1d(float KalmanState, float KalmanUncertainty, float KalmanInput, float KalmanMeasurement) {
// // // //   // KalmanState=KalmanState+0.004*KalmanInput;
// // // //   // KalmanUncertainty=KalmanUncertainty + 0.004 * 0.004 * 4 * 4;


// // // //   KalmanState += dt * KalmanInput;

// // // //   KalmanUncertainty += dt * dt * 4 * 4;
// // // //   float KalmanGain=KalmanUncertainty * 1/(1*KalmanUncertainty + 3 * 3);
// // // //   KalmanState=KalmanState+KalmanGain * (KalmanMeasurement-KalmanState);
// // // //   KalmanUncertainty=(1-KalmanGain) * KalmanUncertainty;
// // // //   Kalman1DOutput[0]=KalmanState; 
// // // //   Kalman1DOutput[1]=KalmanUncertainty;
// // // // }

// // // // // ================= DATA =================
// // // // typedef struct {
// // // //   uint16_t ch[8];
// // // // } Data;

// // // // Data rx;




// // // // // ================= STATE =================
// // // // unsigned long lastRX = 0;

// // // // bool connected = false;
// // // // bool armed = false;

// // // // uint8_t lostCount = 0;

// // // // // ================= ESP-NOW CALLBACK =================
// // // // void onRecv(const uint8_t *mac, const uint8_t *data, int len)
// // // // {
// // // //     memcpy(&rx, data, sizeof(rx));

// // // //     lastRX = millis();
// // // //     connected = true;
// // // //     lostCount = 0;
// // // // }

// // // // void gyro_signals(void) {
// // // // Wire.beginTransmission(0x68);
// // // // Wire.write(0x3B);
// // // // Wire.endTransmission(false);   // Repeated START
// // // // Wire.requestFrom(0x68, 6);
// // // // if(Wire.available()!=6)
// // // //     return;
  
// // // //    AccXLSB = Wire.read() << 8 | Wire.read();
// // // //    AccYLSB = Wire.read() << 8 | Wire.read();
// // // //    AccZLSB = Wire.read() << 8 | Wire.read();                                                
// // // //   Wire.beginTransmission(0x68);
// // // //   Wire.write(0x43);
// // // //   Wire.endTransmission(false);
// // // //   Wire.requestFrom(0x68,6);
// // // //  if(Wire.available()!=6)
// // // //     return;

// // // //    GyroX=Wire.read()<<8 | Wire.read();
// // // //    GyroY=Wire.read()<<8 | Wire.read();
// // // //    GyroZ=Wire.read()<<8 | Wire.read();
// // // //   RateRoll=(float)GyroX/65.5;
// // // //   RatePitch=(float)GyroY/65.5;
// // // //   RateYaw=(float)GyroZ/65.5;

// // // // //   float RateRollRaw  = (float)GyroX / 65.5f;
// // // // // float RatePitchRaw = (float)GyroY / 65.5f;
// // // // // float RateYawRaw   = (float)GyroZ / 65.5f;

// // // // // RateRoll  = 0.7f * RateRoll  + 0.3f * RateRollRaw;
// // // // // RatePitch = 0.7f * RatePitch + 0.3f * RatePitchRaw;
// // // // // RateYaw   = 0.7f * RateYaw   + 0.3f * RateYawRaw;



// // // //   AccX=(float)AccXLSB/4096;
// // // //   AccY=(float)AccYLSB/4096;
// // // //   AccZ=(float)AccZLSB/4096;




// // // // }
// // // // void pid_equation(float Error, float P , float I, float D, float PrevError, float PrevIterm) {
// // // //   float Pterm=P*Error;
// // // //   // float Iterm=PrevIterm+I*(Error+PrevError)*0.004/2;

// // // //     float Iterm =PrevIterm + I * (Error + PrevError) * dt * 0.5f;


// // // //   // if (Iterm > 400) Iterm=400;
// // // //   // else if (Iterm <-400) Iterm=-400;
// // // //     if(Iterm>100) Iterm=100;
// // // //     if(Iterm<-100) Iterm=-100;

// // // //   // float Dterm=D*(Error-PrevError)/0.004;

// // // //   float Dterm =D * (Error - PrevError) / dt;
     
// // // //   float PIDOutput= Pterm+Iterm+Dterm;
// // // //   // if (PIDOutput>400) PIDOutput=400;
// // // //   // else if (PIDOutput <-400) PIDOutput=-400;
// // // //     if(PIDOutput > 100) PIDOutput = 100;
// // // //     if(PIDOutput < -100) PIDOutput = -100;
// // // //   PIDReturn[0]=PIDOutput;
// // // //   PIDReturn[1]=Error;
// // // //   PIDReturn[2]=Iterm;
// // // // }



// // // // void reset_pid(void) {
// // // //   PrevErrorRateRoll=0; PrevErrorRatePitch=0; PrevErrorRateYaw=0;
// // // //   PrevItermRateRoll=0; PrevItermRatePitch=0; PrevItermRateYaw=0;
// // // //   PrevErrorAngleRoll=0; PrevErrorAnglePitch=0;    
// // // //   PrevItermAngleRoll=0; PrevItermAnglePitch=0;
// // // // }
// // // // void setup() {

// // // //     Serial.begin(115200);
// // // //     pinMode(M1, OUTPUT);
// // // //     pinMode(M2, OUTPUT);
// // // //     pinMode(M3, OUTPUT);
// // // //     pinMode(M4, OUTPUT);



// // // //     WiFi.mode(WIFI_STA);

// // // //     if (esp_now_init() != ESP_OK)
// // // //     {
// // // //         Serial.println("ESP-NOW FAIL");
// // // //         return;
// // // //     }

// // // //     esp_now_register_recv_cb(onRecv);

// // // //     Serial.println("RX READY");

// // // //   Wire.setClock(400000);
// // // //   Wire.begin();


// // // //   delay(250);
// // // //   Wire.beginTransmission(0x68);
// // // //   Wire.write(0x6B);
// // // //   Wire.write(0x00);
// // // //   Wire.endTransmission();


// // // //   Wire.beginTransmission(0x68);
// // // //   Wire.write(0x1A);
// // // //   Wire.write(0x05);
// // // //   Wire.endTransmission();

// // // //   // Gyro ±500 dps
// // // //   Wire.beginTransmission(0x68);
// // // //   Wire.write(0x1B);
// // // //   Wire.write(0x08);
// // // //   Wire.endTransmission();

// // // //   // Acc ±8g
// // // //   Wire.beginTransmission(0x68);
// // // //   Wire.write(0x1C);
// // // //   Wire.write(0x10);
// // // //   Wire.endTransmission();


  
// // // //   for (RateCalibrationNumber=0; 
// // // //         RateCalibrationNumber<2000;
// // // //         RateCalibrationNumber ++) {
// // // //           gyro_signals();
// // // //     RateCalibrationGyroRoll+=RateRoll;
// // // //     RateCalibrationGyroPitch+=RatePitch;
// // // //     RateCalibrationGyroYaw+=RateYaw;


// // // //     RateCalibrationAccRoll+=AccX;
// // // //     RateCalibrationAccPitch+=AccY;
// // // //     RateCalibrationAccYaw+=AccZ;
// // // //     delay(1);
// // // //   }
// // // //   RateCalibrationGyroRoll/=2000;
// // // //   RateCalibrationGyroPitch/=2000;
// // // //   RateCalibrationGyroYaw/=2000;


// // // //   // RateCalibrationAccRoll/=2000;
// // // //   // RateCalibrationAccPitch/=2000;
// // // //   // RateCalibrationAccYaw/=2000;

// // // //     float avgX = RateCalibrationAccRoll / 2000;
// // // //     float avgY = RateCalibrationAccPitch / 2000;
// // // //     float avgZ = RateCalibrationAccYaw / 2000.0f;


// // // // //     // Vì đang đặt nằm ngang:
// // // // //     // X = 0g
// // // // //     // Y = 0g
// // // // //     // Z = +1g = 16384

// // // //   RateCalibrationAccRoll = avgX;
// // // // RateCalibrationAccPitch = avgY;
// // // // RateCalibrationAccYaw = avgZ - 1;


// // // //   LoopTimer=micros();
// // // // }

// // // // void loop() {


// // // //    // ================= CHECK LOST TX =================
// // // //     if (connected && millis() - lastRX > 300)
// // // //     {
// // // //         lostCount++;

// // // //         if (lostCount >= 5)
// // // //         {
// // // //             connected = false;

// // // //             if (armed)
// // // //             {
// // // //                 armed = false;
// // // //                  reset_pid();
// // // //                 //  KalmanAngleRoll = 0;
// // // //                 //   KalmanAnglePitch = 0;

// // // //                 //   KalmanUncertaintyAngleRoll = 4;
// // // //                 //   KalmanUncertaintyAnglePitch = 4;
// // // //                 Serial.println("AUTO DISARM");
// // // //             }

// // // //             Serial.println("TX DISCONNECTED");
// // // //         }

// // // //         lastRX = millis();
// // // //     }


// // // //        //         // ARM  khi giam xuong
// // // //       if (connected && !armed && rx.ch[0] < 1050 &&  rx.ch[2] > 1800)
// // // //       {
// // // //           armed = true;

// // // //           reset_pid();
// // // //           KalmanAngleRoll = 0;
// // // //           KalmanAnglePitch = 0;

// // // //           KalmanUncertaintyAngleRoll = 4;
// // // //           KalmanUncertaintyAnglePitch = 4;


// // // //           GyroRollFilter  = RateRoll;
// // // // GyroPitchFilter = RatePitch;
// // // // GyroYawFilter   = RateYaw;
// // // //           Serial.println("ARM");

// // // //       }

    


// // // //       if(armed){

// // // //             gyro_signals();
// // // //             RateRoll-=RateCalibrationGyroRoll;
// // // //             RatePitch-=RateCalibrationGyroPitch;
// // // //             RateYaw-=RateCalibrationGyroYaw;


// // // //                     GyroRollFilter =
// // // //               0.7f * GyroRollFilter +
// // // //               0.3f * RateRoll;

// // // //           GyroPitchFilter =
// // // //               0.7f * GyroPitchFilter +
// // // //               0.3f * RatePitch;

// // // //           GyroYawFilter =
// // // //               0.7f * GyroYawFilter +
// // // //               0.3f * RateYaw;  

// // // //             AccX-=RateCalibrationAccRoll;
// // // //             AccY-=RateCalibrationAccPitch;
// // // //             AccZ-=RateCalibrationAccYaw;


// // // //           AccXFilter = 0.9f * AccXFilter + 0.1f * AccX;
// // // //           AccYFilter = 0.9f * AccYFilter + 0.1f * AccY;
// // // //           AccZFilter = 0.9f * AccZFilter + 0.1f * AccZ;


// // // //           // AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
// // // //           // AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);



// // // //           // AngleRoll =atan2(AccY, sqrt(AccX*AccX + AccZ*AccZ))* 180.0f / PI;
// // // //           // AnglePitch =-atan2(AccX, sqrt(AccY*AccY + AccZ*AccZ))* 180.0f / PI;

// // // //           AngleRoll =
// // // // atan2(
// // // //     AccYFilter,
// // // //     sqrt(
// // // //         AccXFilter * AccXFilter +
// // // //         AccZFilter * AccZFilter
// // // //     )
// // // // ) * 180.0f / PI;

// // // // AnglePitch =
// // // // -atan2(
// // // //     AccXFilter,
// // // //     sqrt(
// // // //         AccYFilter * AccYFilter +
// // // //         AccZFilter * AccZFilter
// // // //     )
// // // // ) * 180.0f / PI;



// // // //             //   Serial.print(" RateRoll:"); Serial.print(RateRoll);
// // // //             // Serial.print(" RatePitch:"); Serial.print(RatePitch);
// // // //             // Serial.print(" RateYaw:"); Serial.print(RateYaw);


// // // //             // Serial.print(" AccX:"); Serial.print(AccX);
// // // //             // Serial.print(" AccY:"); Serial.print(AccY);
// // // //             // Serial.print(" AccZ:"); Serial.println(AccZ);
      


// // // //             kalman_1d(KalmanAngleRoll, KalmanUncertaintyAngleRoll, RateRoll, AngleRoll);
// // // //             KalmanAngleRoll=Kalman1DOutput[0]; KalmanUncertaintyAngleRoll=Kalman1DOutput[1];
// // // //             kalman_1d(KalmanAnglePitch, KalmanUncertaintyAnglePitch, RatePitch, AnglePitch);
// // // //             KalmanAnglePitch=Kalman1DOutput[0]; KalmanUncertaintyAnglePitch=Kalman1DOutput[1];
      

            
// // // //             // DesiredAngleRoll  = 0.10 * (rx.ch[3] - 1500);
// // // //            int roll = rx.ch[3] - 1500;

// // // //             if(abs(roll) < 15)
// // // //                 roll = 0;

// // // //             DesiredAngleRoll = 0.10f * roll;


// // // //           // DesiredAnglePitch = 0.10 * (rx.ch[2] - 1500);
// // // //           int pitch = rx.ch[2]-1500;

// // // //           if(abs(pitch)<15)
// // // //               pitch=0;

// // // //           DesiredAnglePitch=0.10f*pitch;


// // // //           // DesiredRateYaw = 0.08 * (rx.ch[1] - 1500);

// // // //           int yaw = rx.ch[1]-1500;

// // // //           if(abs(yaw)<15)
// // // //               yaw=0;

// // // //           DesiredRateYaw=0.08f*yaw;

  
// // // //           // InputThrottle = map(rx.ch[0], 1000, 2000, 0, 255);
// // // //           InputThrottle =(rx.ch[0]-1000)*255.0f/1000.0f;
// // // //           InputThrottle = constrain(InputThrottle, 0, 255);

          

             

// // // //             ErrorAngleRoll=DesiredAngleRoll-KalmanAngleRoll;
// // // //             ErrorAnglePitch=DesiredAnglePitch-KalmanAnglePitch;
// // // //             pid_equation(ErrorAngleRoll, PAngleRoll, IAngleRoll, DAngleRoll, PrevErrorAngleRoll, PrevItermAngleRoll);     
// // // //             DesiredRateRoll=PIDReturn[0]; 
// // // //             PrevErrorAngleRoll=PIDReturn[1];
// // // //             PrevItermAngleRoll=PIDReturn[2];
// // // //             pid_equation(ErrorAnglePitch, PAnglePitch, IAnglePitch, DAnglePitch, PrevErrorAnglePitch, PrevItermAnglePitch);
// // // //             DesiredRatePitch=PIDReturn[0]; 
// // // //             PrevErrorAnglePitch=PIDReturn[1];
// // // //             PrevItermAnglePitch=PIDReturn[2];
// // // //             // ErrorRateRoll=DesiredRateRoll-RateRoll;
// // // //             // ErrorRatePitch=DesiredRatePitch-RatePitch;
// // // //             // ErrorRateYaw=DesiredRateYaw-RateYaw;
// // // //              ErrorRateRoll  = DesiredRateRoll  - GyroRollFilter;
// // // // ErrorRatePitch = DesiredRatePitch - GyroPitchFilter;
// // // // ErrorRateYaw   = DesiredRateYaw   - GyroYawFilter;

// // // //             pid_equation(ErrorRateRoll, PRateRoll, IRateRoll, DRateRoll, PrevErrorRateRoll, PrevItermRateRoll);
// // // //                  InputRoll=PIDReturn[0];
// // // //                  PrevErrorRateRoll=PIDReturn[1]; 
// // // //                  PrevItermRateRoll=PIDReturn[2];
// // // //             pid_equation(ErrorRatePitch, PRatePitch,IRatePitch, DRatePitch, PrevErrorRatePitch, PrevItermRatePitch);
// // // //                  InputPitch=PIDReturn[0]; 
// // // //                  PrevErrorRatePitch=PIDReturn[1]; 
// // // //                  PrevItermRatePitch=PIDReturn[2];
// // // //             pid_equation(ErrorRateYaw, PRateYaw,IRateYaw, DRateYaw, PrevErrorRateYaw, PrevItermRateYaw);
// // // //                  InputYaw=PIDReturn[0]; 
// // // //                  PrevErrorRateYaw=PIDReturn[1]; 
// // // //                  PrevItermRateYaw=PIDReturn[2];

// // // //             InputRoll  = constrain(InputRoll,  -60, 60);
// // // //             InputPitch = constrain(InputPitch, -60, 60);
// // // //             InputYaw   = constrain(InputYaw,   -40, 40);




// // // //             // Serial.print(" InputRoll:"); Serial.print(InputRoll);
// // // //             // Serial.print(" InputPitch:"); Serial.print(InputPitch);
// // // //             // Serial.print(" InputYaw:"); Serial.println(InputYaw);
          

           
// // // //           MotorInput1 = 1.024*(InputThrottle + InputRoll - InputPitch - InputYaw);
// // // //           MotorInput2 = 1.024*(InputThrottle - InputRoll - InputPitch + InputYaw);
// // // //           MotorInput3 = 1.024*(InputThrottle - InputRoll + InputPitch - InputYaw);
// // // //           MotorInput4 = 1.024*(InputThrottle + InputRoll + InputPitch + InputYaw);


// // // //           MotorInput1 = constrain(MotorInput1,0,255);
// // // //           MotorInput2 = constrain(MotorInput2,0,255);
// // // //           MotorInput3 = constrain(MotorInput3,0,255);
// // // //           MotorInput4 = constrain(MotorInput4,0,255);

// // // //           if(rx.ch[0] < 1050)
// // // //           {
// // // //               MotorInput1 = 0;
// // // //               MotorInput2 = 0;
// // // //               MotorInput3 = 0;
// // // //               MotorInput4 = 0;
// // // //               reset_pid();
// // // //               PrevItermRateRoll=0;
// // // //               PrevItermRatePitch=0;
// // // //               PrevItermRateYaw=0;
// // // //           }

// // // //             analogWrite(M1, MotorInput1);
// // // //             analogWrite(M2, MotorInput2);
// // // //             analogWrite(M3, MotorInput3);
// // // //             analogWrite(M4, MotorInput4);

     


          
        

// // // //               Serial.print(" M1:"); Serial.print(rx.ch[0]);
// // // //             Serial.print(" M2:"); Serial.print(rx.ch[1]);
// // // //             Serial.print(" M3:"); Serial.print(rx.ch[2]);
// // // //             Serial.print(" M4:"); Serial.print(rx.ch[3]);
// // // //             Serial.print(" M5:"); Serial.println(rx.ch[4]);
            


          

// // // //             Serial.print(" M1:"); Serial.print(MotorInput1);
// // // //             Serial.print(" M2:"); Serial.print(MotorInput2);
// // // //             Serial.print(" M3:"); Serial.print(MotorInput3);
// // // //             Serial.print(" M4:"); Serial.println(MotorInput4);


          
// // // //       }

// // // //       if(!armed)
// // // //       {
// // // //           reset_pid();

// // // //           MotorInput1=0;
// // // //           MotorInput2=0;
// // // //           MotorInput3=0;
// // // //           MotorInput4=0;

// // // //           analogWrite(M1,0);
// // // //           analogWrite(M2,0);
// // // //           analogWrite(M3,0);
// // // //           analogWrite(M4,0);


// // // //           reset_pid();
      
// // // //       }

// // // //         // while (micros() - LoopTimer < 4000);
// // // //         // LoopTimer=micros();


// // // //         while (micros() - LoopTimer < 4000);

// // // //           uint32_t now = micros();

        

// // // //       dt = (now - LoopTimer) / 1000000.0f;

// // // //       // Giới hạn dt để PID luôn ổn định
// // // //       dt = constrain(dt, 0.001f, 0.01f);

// // // //       LoopTimer = now;

// // // // }





// // // // // #include <Arduino.h>
// // // // // #include <Wire.h>

// // // // // // ======================================================
// // // // // // MPU6050
// // // // // // ======================================================

// // // // // #define MPU6050_ADDR 0x68

// // // // // // ======================================================
// // // // // // RAW SENSOR
// // // // // // ======================================================

// // // // // int16_t AccXLSB;
// // // // // int16_t AccYLSB;
// // // // // int16_t AccZLSB;

// // // // // int16_t GyroX;
// // // // // int16_t GyroY;
// // // // // int16_t GyroZ;

// // // // // // ======================================================
// // // // // // SENSOR VALUE
// // // // // // ======================================================

// // // // // float AccX;
// // // // // float AccY;
// // // // // float AccZ;

// // // // // float RateRoll;
// // // // // float RatePitch;
// // // // // float RateYaw;

// // // // // // ======================================================
// // // // // // CALIBRATION
// // // // // // ======================================================

// // // // // float RateCalibrationGyroRoll  = 0.0f;
// // // // // float RateCalibrationGyroPitch = 0.0f;
// // // // // float RateCalibrationGyroYaw   = 0.0f;

// // // // // float RateCalibrationAccRoll  = 0.0f;
// // // // // float RateCalibrationAccPitch = 0.0f;
// // // // // float RateCalibrationAccYaw   = 0.0f;

// // // // // // ======================================================
// // // // // // LOW PASS FILTER
// // // // // // ======================================================

// // // // // float AccXFilter = 0.0f;
// // // // // float AccYFilter = 0.0f;
// // // // // float AccZFilter = 1.0f;

// // // // // // Filter:
// // // // // // 0.9 = giữ dữ liệu cũ nhiều -> mượt hơn
// // // // // // 0.1 = lấy dữ liệu mới ít
// // // // // //
// // // // // // filtered = 0.9 * old + 0.1 * new

// // // // // const float FILTER_ALPHA = 0.1f;

// // // // // // ======================================================
// // // // // // ANGLE
// // // // // // ======================================================

// // // // // float AngleRoll;
// // // // // float AnglePitch;

// // // // // // ======================================================
// // // // // // KALMAN
// // // // // // ======================================================

// // // // // float dt = 0.004f;

// // // // // float KalmanAngleRoll = 0.0f;
// // // // // float KalmanUncertaintyAngleRoll = 4.0f;

// // // // // float KalmanAnglePitch = 0.0f;
// // // // // float KalmanUncertaintyAnglePitch = 4.0f;

// // // // // float Kalman1DOutput[2] = {0.0f, 0.0f};

// // // // // // ======================================================
// // // // // // LOOP TIMER
// // // // // // ======================================================

// // // // // uint32_t LoopTimer;

// // // // // // ======================================================
// // // // // // KALMAN 1D
// // // // // // ======================================================

// // // // // void kalman_1d(
// // // // //     float KalmanState,
// // // // //     float KalmanUncertainty,
// // // // //     float KalmanInput,
// // // // //     float KalmanMeasurement
// // // // // )
// // // // // {
// // // // //     // Prediction

// // // // //     KalmanState += dt * KalmanInput;

// // // // //     KalmanUncertainty +=
// // // // //         dt * dt * 4.0f * 4.0f;

// // // // //     // Kalman gain

// // // // //     float KalmanGain =
// // // // //         KalmanUncertainty /
// // // // //         (KalmanUncertainty + 3.0f * 3.0f);

// // // // //     // Correction

// // // // //     KalmanState =
// // // // //         KalmanState +
// // // // //         KalmanGain *
// // // // //         (KalmanMeasurement - KalmanState);

// // // // //     KalmanUncertainty =
// // // // //         (1.0f - KalmanGain) *
// // // // //         KalmanUncertainty;

// // // // //     // Output

// // // // //     Kalman1DOutput[0] = KalmanState;
// // // // //     Kalman1DOutput[1] = KalmanUncertainty;
// // // // // }

// // // // // // ======================================================
// // // // // // READ MPU6050
// // // // // // ======================================================

// // // // // bool gyro_signals()
// // // // // {
// // // // //     // ==================================================
// // // // //     // ACCELEROMETER
// // // // //     // ==================================================

// // // // //     Wire.beginTransmission(MPU6050_ADDR);

// // // // //     Wire.write(0x3B);

// // // // //     if (Wire.endTransmission(false) != 0)
// // // // //     {
// // // // //         return false;
// // // // //     }

// // // // //     if (Wire.requestFrom(MPU6050_ADDR, 6) != 6)
// // // // //     {
// // // // //         return false;
// // // // //     }

// // // // //     AccXLSB =
// // // // //         (int16_t)(Wire.read() << 8 | Wire.read());

// // // // //     AccYLSB =
// // // // //         (int16_t)(Wire.read() << 8 | Wire.read());

// // // // //     AccZLSB =
// // // // //         (int16_t)(Wire.read() << 8 | Wire.read());


// // // // //     // ==================================================
// // // // //     // GYROSCOPE
// // // // //     // ==================================================

// // // // //     Wire.beginTransmission(MPU6050_ADDR);

// // // // //     Wire.write(0x43);

// // // // //     if (Wire.endTransmission(false) != 0)
// // // // //     {
// // // // //         return false;
// // // // //     }

// // // // //     if (Wire.requestFrom(MPU6050_ADDR, 6) != 6)
// // // // //     {
// // // // //         return false;
// // // // //     }

// // // // //     GyroX =
// // // // //         (int16_t)(Wire.read() << 8 | Wire.read());

// // // // //     GyroY =
// // // // //         (int16_t)(Wire.read() << 8 | Wire.read());

// // // // //     GyroZ =
// // // // //         (int16_t)(Wire.read() << 8 | Wire.read());


// // // // //     // ==================================================
// // // // //     // CONVERT GYRO
// // // // //     //
// // // // //     // ±500 dps
// // // // //     // 65.5 LSB / dps
// // // // //     // ==================================================

// // // // //     RateRoll =
// // // // //         (float)GyroX / 65.5f;

// // // // //     RatePitch =
// // // // //         (float)GyroY / 65.5f;

// // // // //     RateYaw =
// // // // //         (float)GyroZ / 65.5f;


// // // // //     // ==================================================
// // // // //     // CONVERT ACC
// // // // //     //
// // // // //     // ±8g
// // // // //     // 4096 LSB / g
// // // // //     // ==================================================

// // // // //     AccX =
// // // // //         (float)AccXLSB / 4096.0f;

// // // // //     AccY =
// // // // //         (float)AccYLSB / 4096.0f;

// // // // //     AccZ =
// // // // //         (float)AccZLSB / 4096.0f;


// // // // //     return true;
// // // // // }

// // // // // // ======================================================
// // // // // // CALIBRATION
// // // // // // ======================================================

// // // // // void calibrate_sensor()
// // // // // {
// // // // //     const int SAMPLES = 2000;

// // // // //     float gyroXSum = 0.0f;
// // // // //     float gyroYSum = 0.0f;
// // // // //     float gyroZSum = 0.0f;

// // // // //     float accXSum = 0.0f;
// // // // //     float accYSum = 0.0f;
// // // // //     float accZSum = 0.0f;

// // // // //     int validSamples = 0;

// // // // //     Serial.println();
// // // // //     Serial.println("================================");
// // // // //     Serial.println("START CALIBRATION");
// // // // //     Serial.println("Keep MPU6050 STILL");
// // // // //     Serial.println("================================");

// // // // //     while (validSamples < SAMPLES)
// // // // //     {
// // // // //         if (gyro_signals())
// // // // //         {
// // // // //             gyroXSum += RateRoll;
// // // // //             gyroYSum += RatePitch;
// // // // //             gyroZSum += RateYaw;

// // // // //             accXSum += AccX;
// // // // //             accYSum += AccY;
// // // // //             accZSum += AccZ;

// // // // //             validSamples++;
// // // // //         }

// // // // //         delay(1);
// // // // //     }

// // // // //     // ==================================================
// // // // //     // GYRO OFFSET
// // // // //     // ==================================================

// // // // //     RateCalibrationGyroRoll =
// // // // //         gyroXSum / SAMPLES;

// // // // //     RateCalibrationGyroPitch =
// // // // //         gyroYSum / SAMPLES;

// // // // //     RateCalibrationGyroYaw =
// // // // //         gyroZSum / SAMPLES;


// // // // //     // ==================================================
// // // // //     // ACC OFFSET
// // // // //     //
// // // // //     // Board nằm ngang:
// // // // //     //
// // // // //     // X ≈ 0g
// // // // //     // Y ≈ 0g
// // // // //     // Z ≈ +1g
// // // // //     // ==================================================

// // // // //     float avgX =
// // // // //         accXSum / SAMPLES;

// // // // //     float avgY =
// // // // //         accYSum / SAMPLES;

// // // // //     float avgZ =
// // // // //         accZSum / SAMPLES;


// // // // //     RateCalibrationAccRoll =
// // // // //         avgX;

// // // // //     RateCalibrationAccPitch =
// // // // //         avgY;

// // // // //     RateCalibrationAccYaw =
// // // // //         avgZ - 1.0f;


// // // // //     // ==================================================
// // // // //     // PRINT CALIBRATION
// // // // //     // ==================================================

// // // // //     Serial.println();

// // // // //     Serial.println("===== CALIBRATION =====");

// // // // //     Serial.print("Gyro Offset X: ");
// // // // //     Serial.println(RateCalibrationGyroRoll, 6);

// // // // //     Serial.print("Gyro Offset Y: ");
// // // // //     Serial.println(RateCalibrationGyroPitch, 6);

// // // // //     Serial.print("Gyro Offset Z: ");
// // // // //     Serial.println(RateCalibrationGyroYaw, 6);

// // // // //     Serial.print("Acc Offset X: ");
// // // // //     Serial.println(RateCalibrationAccRoll, 6);

// // // // //     Serial.print("Acc Offset Y: ");
// // // // //     Serial.println(RateCalibrationAccPitch, 6);

// // // // //     Serial.print("Acc Offset Z: ");
// // // // //     Serial.println(RateCalibrationAccYaw, 6);

// // // // //     Serial.println("=======================");
// // // // //     Serial.println();


// // // // //     // ==================================================
// // // // //     // RESET FILTER
// // // // //     // ==================================================

// // // // //     AccXFilter = 0.0f;
// // // // //     AccYFilter = 0.0f;
// // // // //     AccZFilter = 1.0f;

// // // // //     // ==================================================
// // // // //     // RESET KALMAN
// // // // //     // ==================================================

// // // // //     KalmanAngleRoll = 0.0f;
// // // // //     KalmanAnglePitch = 0.0f;

// // // // //     KalmanUncertaintyAngleRoll = 4.0f;
// // // // //     KalmanUncertaintyAnglePitch = 4.0f;
// // // // // }

// // // // // // ======================================================
// // // // // // SETUP
// // // // // // ======================================================

// // // // // void setup()
// // // // // {
// // // // //     Serial.begin(115200);

// // // // //     delay(1000);

// // // // //     Serial.println();
// // // // //     Serial.println("MPU6050 FILTER TEST");

// // // // //     // ==================================================
// // // // //     // I2C
// // // // //     // ==================================================

// // // // //     Wire.begin();

// // // // //     Wire.setClock(400000);

// // // // //     delay(100);


// // // // //     // ==================================================
// // // // //     // WAKE MPU6050
// // // // //     // ==================================================

// // // // //     Wire.beginTransmission(MPU6050_ADDR);

// // // // //     Wire.write(0x6B);
// // // // //     Wire.write(0x00);

// // // // //     Wire.endTransmission();


// // // // //     // ==================================================
// // // // //     // CONFIG
// // // // //     // ==================================================

// // // // //     // DLPF

// // // // //     Wire.beginTransmission(MPU6050_ADDR);

// // // // //     Wire.write(0x1A);
// // // // //     Wire.write(0x05);

// // // // //     Wire.endTransmission();


// // // // //     // ==================================================
// // // // //     // GYRO ±500 dps
// // // // //     // ==================================================

// // // // //     Wire.beginTransmission(MPU6050_ADDR);

// // // // //     Wire.write(0x1B);
// // // // //     Wire.write(0x08);

// // // // //     Wire.endTransmission();


// // // // //     // ==================================================
// // // // //     // ACC ±8g
// // // // //     // ==================================================

// // // // //     Wire.beginTransmission(MPU6050_ADDR);

// // // // //     Wire.write(0x1C);
// // // // //     Wire.write(0x10);

// // // // //     Wire.endTransmission();


// // // // //     delay(250);


// // // // //     // ==================================================
// // // // //     // CALIBRATION
// // // // //     // ==================================================

// // // // //     calibrate_sensor();


// // // // //     // ==================================================
// // // // //     // TIMER
// // // // //     // ==================================================

// // // // //     LoopTimer = micros();

// // // // //     Serial.println("READY");
// // // // // }

// // // // // // ======================================================
// // // // // // LOOP
// // // // // // ======================================================

// // // // // void loop()
// // // // // {
// // // // //     // ==================================================
// // // // //     // 250 Hz
// // // // //     // ==================================================

// // // // //     while (micros() - LoopTimer < 4000)
// // // // //     {
// // // // //         // wait
// // // // //     }


// // // // //     uint32_t now = micros();


// // // // //     // ==================================================
// // // // //     // DT
// // // // //     // ==================================================

// // // // //     dt =
// // // // //         (now - LoopTimer) *
// // // // //         0.000001f;

// // // // //     if (dt < 0.001f)
// // // // //         dt = 0.001f;

// // // // //     if (dt > 0.01f)
// // // // //         dt = 0.01f;

// // // // //     LoopTimer = now;


// // // // //     // ==================================================
// // // // //     // READ SENSOR
// // // // //     // ==================================================

// // // // //     if (!gyro_signals())
// // // // //     {
// // // // //         Serial.println("MPU6050 I2C ERROR");
// // // // //         return;
// // // // //     }


// // // // //     // ==================================================
// // // // //     // REMOVE CALIBRATION OFFSET
// // // // //     // ==================================================

// // // // //     float GyroRollFiltered =
// // // // //         RateRoll - RateCalibrationGyroRoll;

// // // // //     float GyroPitchFiltered =
// // // // //         RatePitch - RateCalibrationGyroPitch;

// // // // //     float GyroYawFiltered =
// // // // //         RateYaw - RateCalibrationGyroYaw;


// // // // //     float AccXCalibrated =
// // // // //         AccX - RateCalibrationAccRoll;

// // // // //     float AccYCalibrated =
// // // // //         AccY - RateCalibrationAccPitch;

// // // // //     float AccZCalibrated =
// // // // //         AccZ - RateCalibrationAccYaw;


// // // // //     // ==================================================
// // // // //     // LOW PASS FILTER ACC
// // // // //     // ==================================================

// // // // //     AccXFilter =
// // // // //         (1.0f - FILTER_ALPHA) * AccXFilter
// // // // //         + FILTER_ALPHA * AccXCalibrated;

// // // // //     AccYFilter =
// // // // //         (1.0f - FILTER_ALPHA) * AccYFilter
// // // // //         + FILTER_ALPHA * AccYCalibrated;

// // // // //     AccZFilter =
// // // // //         (1.0f - FILTER_ALPHA) * AccZFilter
// // // // //         + FILTER_ALPHA * AccZCalibrated;


// // // // //     // ==================================================
// // // // //     // CALCULATE ANGLE FROM FILTERED ACC
// // // // //     // ==================================================

// // // // //     AngleRoll =
// // // // //         atan2(
// // // // //             AccYFilter,
// // // // //             sqrt(
// // // // //                 AccXFilter * AccXFilter +
// // // // //                 AccZFilter * AccZFilter
// // // // //             )
// // // // //         )
// // // // //         * 180.0f / PI;


// // // // //     AnglePitch =
// // // // //         -atan2(
// // // // //             AccXFilter,
// // // // //             sqrt(
// // // // //                 AccYFilter * AccYFilter +
// // // // //                 AccZFilter * AccZFilter
// // // // //             )
// // // // //         )
// // // // //         * 180.0f / PI;


// // // // //     // ==================================================
// // // // //     // KALMAN ROLL
// // // // //     // ==================================================

// // // // //     kalman_1d(
// // // // //         KalmanAngleRoll,
// // // // //         KalmanUncertaintyAngleRoll,
// // // // //         GyroRollFiltered,
// // // // //         AngleRoll
// // // // //     );

// // // // //     KalmanAngleRoll =
// // // // //         Kalman1DOutput[0];

// // // // //     KalmanUncertaintyAngleRoll =
// // // // //         Kalman1DOutput[1];


// // // // //     // ==================================================
// // // // //     // KALMAN PITCH
// // // // //     // ==================================================

// // // // //     kalman_1d(
// // // // //         KalmanAnglePitch,
// // // // //         KalmanUncertaintyAnglePitch,
// // // // //         GyroPitchFiltered,
// // // // //         AnglePitch
// // // // //     );

// // // // //     KalmanAnglePitch =
// // // // //         Kalman1DOutput[0];

// // // // //     KalmanUncertaintyAnglePitch =
// // // // //         Kalman1DOutput[1];


// // // // //     // ==================================================
// // // // //     // SERIAL LOG
// // // // //     //
// // // // //     // Chỉ log mỗi 100ms
// // // // //     // Không log 250 lần/s
// // // // //     // ==================================================

// // // // //     static uint32_t lastLog = 0;

// // // // //     if (millis() - lastLog >= 100)
// // // // //     {
// // // // //         lastLog = millis();


// // // // //         Serial.print("GYRO_CAL");

// // // // //         Serial.print(" Roll=");
// // // // //         Serial.print(GyroRollFiltered, 3);

// // // // //         Serial.print(" Pitch=");
// // // // //         Serial.print(GyroPitchFiltered, 3);

// // // // //         Serial.print(" Yaw=");
// // // // //         Serial.print(GyroYawFiltered, 3);


// // // // //         Serial.print(" | ACC_CAL");

// // // // //         Serial.print(" X=");
// // // // //         Serial.print(AccXCalibrated, 3);

// // // // //         Serial.print(" Y=");
// // // // //         Serial.print(AccYCalibrated, 3);

// // // // //         Serial.print(" Z=");
// // // // //         Serial.print(AccZCalibrated, 3);


// // // // //         Serial.print(" | ACC_FILTER");

// // // // //         Serial.print(" X=");
// // // // //         Serial.print(AccXFilter, 3);

// // // // //         Serial.print(" Y=");
// // // // //         Serial.print(AccYFilter, 3);

// // // // //         Serial.print(" Z=");
// // // // //         Serial.print(AccZFilter, 3);


// // // // //         Serial.print(" | ANGLE");

// // // // //         Serial.print(" Roll=");
// // // // //         Serial.print(AngleRoll, 2);

// // // // //         Serial.print(" Pitch=");
// // // // //         Serial.print(AnglePitch, 2);


// // // // //         Serial.print(" | KALMAN");

// // // // //         Serial.print(" Roll=");
// // // // //         Serial.print(KalmanAngleRoll, 2);

// // // // //         Serial.print(" Pitch=");
// // // // //         Serial.print(KalmanAnglePitch, 2);


// // // // //         Serial.print(" | dt=");
// // // // //         Serial.println(dt, 6);
// // // // //     }
// // // // // }







































// // // // // #include <Arduino.h>
// // // // // #include <Wire.h>
// // // // // #include <WiFi.h>
// // // // // #include <esp_now.h>

// // // // // // Chân PWM cho từng motor
// // // // // int M1 = 3;
// // // // // int M2 = 6;
// // // // // int M3 = 5;
// // // // // int M4 = 4;


// // // // // float RateRoll, RatePitch, RateYaw;
// // // // // float RateCalibrationGyroRoll, RateCalibrationGyroPitch, RateCalibrationGyroYaw;


// // // // // float RateCalibrationAccRoll, RateCalibrationAccPitch, RateCalibrationAccYaw;
// // // // // int RateCalibrationNumber;
// // // // // float ReceiverValue[]={0, 0, 0, 0, 0, 0, 0, 0};
// // // // // int ChannelNumber=0; 
// // // // // float Voltage, Current, BatteryRemaining, BatteryAtStart;
// // // // // float CurrentConsumed=0;
// // // // // float BatteryDefault=1300;
// // // // // uint32_t LoopTimer;
// // // // // float DesiredRateRoll, DesiredRatePitch,DesiredRateYaw;
// // // // // float ErrorRateRoll, ErrorRatePitch, ErrorRateYaw;
// // // // // float InputRoll, InputThrottle, InputPitch, InputYaw;
// // // // // float PrevErrorRateRoll, PrevErrorRatePitch, PrevErrorRateYaw;
// // // // // float PrevItermRateRoll, PrevItermRatePitch, PrevItermRateYaw;
// // // // // float PIDReturn[]={0, 0, 0};
// // // // // // float PRateRoll= 1.14; float PRatePitch=PRateRoll; float PRateYaw=0;
// // // // // // float IRateRoll=0.05; float IRatePitch=IRateRoll; float IRateYaw=0.0;
// // // // // // float DRateRoll=0.03; float DRatePitch=DRateRoll; float DRateYaw=0.0;
// // // // // float PRateRoll= 1.14; float PRatePitch=PRateRoll; float PRateYaw=0;
// // // // // float IRateRoll=0.08; float IRatePitch=IRateRoll; float IRateYaw=0;
// // // // // float DRateRoll=0.03; float DRatePitch=DRateRoll; float DRateYaw=0;
// // // // // float MotorInput1, MotorInput2, MotorInput3, MotorInput4;
// // // // // float AccX, AccY, AccZ;
// // // // // float AngleRoll, AnglePitch;
// // // // // float KalmanAngleRoll=0, KalmanUncertaintyAngleRoll=2*2;
// // // // // float KalmanAnglePitch=0, KalmanUncertaintyAnglePitch=2*2;
// // // // // float Kalman1DOutput[]={0,0};
// // // // // float DesiredAngleRoll, DesiredAnglePitch;
// // // // // float ErrorAngleRoll, ErrorAnglePitch;
// // // // // float PrevErrorAngleRoll, PrevErrorAnglePitch;
// // // // // float PrevItermAngleRoll, PrevItermAnglePitch;
// // // // // float PAngleRoll=1.14; float PAnglePitch=PAngleRoll;
// // // // // float IAngleRoll=0.08; float IAnglePitch=IAngleRoll;
// // // // // float DAngleRoll=0.02; float DAnglePitch=DAngleRoll;

// // // // // int16_t AccXLSB;
// // // // // int16_t AccYLSB;
// // // // // int16_t AccZLSB;
// // // // // int16_t GyroX;
// // // // // int16_t GyroY;
// // // // // int16_t GyroZ;


// // // // // void kalman_1d(float KalmanState, float KalmanUncertainty, float KalmanInput, float KalmanMeasurement) {
// // // // //   KalmanState=KalmanState+0.004*KalmanInput;
// // // // //   KalmanUncertainty=KalmanUncertainty + 0.004 * 0.004 * 4 * 4;
// // // // //   float KalmanGain=KalmanUncertainty * 1/(1*KalmanUncertainty + 3 * 3);
// // // // //   KalmanState=KalmanState+KalmanGain * (KalmanMeasurement-KalmanState);
// // // // //   KalmanUncertainty=(1-KalmanGain) * KalmanUncertainty;
// // // // //   Kalman1DOutput[0]=KalmanState; 
// // // // //   Kalman1DOutput[1]=KalmanUncertainty;
// // // // // }

// // // // // // ================= DATA =================
// // // // // typedef struct {
// // // // //   uint16_t ch[8];
// // // // // } Data;

// // // // // Data rx;




// // // // // // ================= STATE =================
// // // // // unsigned long lastRX = 0;

// // // // // bool connected = false;
// // // // // bool armed = false;

// // // // // uint8_t lostCount = 0;

// // // // // // ================= ESP-NOW CALLBACK =================
// // // // // void onRecv(const uint8_t *mac, const uint8_t *data, int len)
// // // // // {
// // // // //     memcpy(&rx, data, sizeof(rx));

// // // // //     lastRX = millis();
// // // // //     connected = true;
// // // // //     lostCount = 0;


// // // // //      Serial.printf(
// // // // //         "RX -> CH0:%4d CH1:%4d CH2:%4d CH3:%4d CH4:%4d CH5:%4d CH6:%4d CH7:%4d\n",
// // // // //         rx.ch[0],
// // // // //         rx.ch[1],
// // // // //         rx.ch[2],
// // // // //         rx.ch[3],
// // // // //         rx.ch[4],
// // // // //         rx.ch[5],
// // // // //         rx.ch[6],
// // // // //         rx.ch[7]
// // // // //     );
// // // // // }

// // // // // void gyro_signals(void) {
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1A);
// // // // //   Wire.write(0x05);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1C);
// // // // //   Wire.write(0x10);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x3B);
// // // // //   Wire.endTransmission(); 
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   // int16_t AccXLSB = Wire.read() << 8 | Wire.read();
// // // // //   // int16_t AccYLSB = Wire.read() << 8 | Wire.read();
// // // // //   // int16_t AccZLSB = Wire.read() << 8 | Wire.read();

// // // // //    AccXLSB = Wire.read() << 8 | Wire.read();
// // // // //    AccYLSB = Wire.read() << 8 | Wire.read();
// // // // //    AccZLSB = Wire.read() << 8 | Wire.read();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1B); 
// // // // //   Wire.write(0x8);
// // // // //   Wire.endTransmission();                                                   
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x43);
// // // // //   Wire.endTransmission();
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   // int16_t GyroX=Wire.read()<<8 | Wire.read();
// // // // //   // int16_t GyroY=Wire.read()<<8 | Wire.read();
// // // // //   // int16_t GyroZ=Wire.read()<<8 | Wire.read();


// // // // //    GyroX=Wire.read()<<8 | Wire.read();
// // // // //    GyroY=Wire.read()<<8 | Wire.read();
// // // // //    GyroZ=Wire.read()<<8 | Wire.read();
// // // // //   RateRoll=(float)GyroX/65.5;
// // // // //   RatePitch=(float)GyroY/65.5;
// // // // //   RateYaw=(float)GyroZ/65.5;
// // // // //   AccX=(float)AccXLSB/4096;
// // // // //   AccY=(float)AccYLSB/4096;
// // // // //   AccZ=(float)AccZLSB/4096;
// // // // //   // AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
// // // // //   // AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);
// // // // // }
// // // // // void pid_equation(float Error, float P , float I, float D, float PrevError, float PrevIterm) {
// // // // //   float Pterm=P*Error;
// // // // //   float Iterm=PrevIterm+I*(Error+PrevError)*0.004/2;
// // // // //   // if (Iterm > 400) Iterm=400;
// // // // //   // else if (Iterm <-400) Iterm=-400;
// // // // //     if(Iterm>100) Iterm=100;
// // // // //     if(Iterm<-100) Iterm=-100;

// // // // //   float Dterm=D*(Error-PrevError)/0.004;
     
// // // // //   float PIDOutput= Pterm+Iterm+Dterm;
// // // // //   // if (PIDOutput>400) PIDOutput=400;
// // // // //   // else if (PIDOutput <-400) PIDOutput=-400;
// // // // //     if(PIDOutput > 100) PIDOutput = 100;
// // // // //     if(PIDOutput < -100) PIDOutput = -100;
// // // // //   PIDReturn[0]=PIDOutput;
// // // // //   PIDReturn[1]=Error;
// // // // //   PIDReturn[2]=Iterm;
// // // // // }



// // // // // void reset_pid(void) {
// // // // //   PrevErrorRateRoll=0; PrevErrorRatePitch=0; PrevErrorRateYaw=0;
// // // // //   PrevItermRateRoll=0; PrevItermRatePitch=0; PrevItermRateYaw=0;
// // // // //   PrevErrorAngleRoll=0; PrevErrorAnglePitch=0;    
// // // // //   PrevItermAngleRoll=0; PrevItermAnglePitch=0;
// // // // // }
// // // // // void setup() {

// // // // //     Serial.begin(115200);
// // // // //     pinMode(M1, OUTPUT);
// // // // //     pinMode(M2, OUTPUT);
// // // // //     pinMode(M3, OUTPUT);
// // // // //     pinMode(M4, OUTPUT);

// // // // // //    ledcSetup(CH1, 400, 8);
// // // // // // ledcSetup(CH2, 400, 8);
// // // // // // ledcSetup(CH3, 400, 8);
// // // // // // ledcSetup(CH4, 400, 8);

// // // // // // ledcAttachPin(M1, CH1);
// // // // // // ledcAttachPin(M2, CH2);
// // // // // // ledcAttachPin(M3, CH3);
// // // // // // ledcAttachPin(M4, CH4);

// // // // //     WiFi.mode(WIFI_STA);

// // // // //     if (esp_now_init() != ESP_OK)
// // // // //     {
// // // // //         Serial.println("ESP-NOW FAIL");
// // // // //         return;
// // // // //     }

// // // // //     esp_now_register_recv_cb(onRecv);

// // // // //     Serial.println("RX READY");

// // // // //   Wire.setClock(400000);
// // // // //   Wire.begin();
// // // // //   delay(250);
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x6B);
// // // // //   Wire.write(0x00);
// // // // //   Wire.endTransmission();
// // // // //   for (RateCalibrationNumber=0; 
// // // // //         RateCalibrationNumber<2000;
// // // // //         RateCalibrationNumber ++) {
// // // // //           gyro_signals();
// // // // //     RateCalibrationGyroRoll+=RateRoll;
// // // // //     RateCalibrationGyroPitch+=RatePitch;
// // // // //     RateCalibrationGyroYaw+=RateYaw;


// // // // //     RateCalibrationAccRoll+=AccX;
// // // // //     RateCalibrationAccPitch+=AccY;
// // // // //     RateCalibrationAccYaw+=AccZ;
// // // // //     delay(1);
// // // // //   }
// // // // //   RateCalibrationGyroRoll/=2000;
// // // // //   RateCalibrationGyroPitch/=2000;
// // // // //   RateCalibrationGyroYaw/=2000;


// // // // //   // RateCalibrationAccRoll/=2000;
// // // // //   // RateCalibrationAccPitch/=2000;
// // // // //   // RateCalibrationAccYaw/=2000;

// // // // //     float avgX = RateCalibrationAccRoll / 2000;
// // // // //     float avgY = RateCalibrationAccPitch / 2000;
// // // // //     float avgZ =   RateCalibrationAccYaw/=2000;


// // // // // //     // Vì đang đặt nằm ngang:
// // // // // //     // X = 0g
// // // // // //     // Y = 0g
// // // // // //     // Z = +1g = 16384

// // // // //   RateCalibrationAccRoll = avgX;
// // // // // RateCalibrationAccPitch = avgY;
// // // // // RateCalibrationAccYaw = avgZ - 1;


// // // // //   LoopTimer=micros();
// // // // // }

// // // // // void loop() {


// // // // //    // ================= CHECK LOST TX =================
// // // // //     if (connected && millis() - lastRX > 300)
// // // // //     {
// // // // //         lostCount++;

// // // // //         if (lostCount >= 5)
// // // // //         {
// // // // //             connected = false;

// // // // //             if (armed)
// // // // //             {
// // // // //                 armed = false;
// // // // //                  reset_pid();
// // // // //                  KalmanAngleRoll = 0;
// // // // //                   KalmanAnglePitch = 0;

// // // // //                   KalmanUncertaintyAngleRoll = 4;
// // // // //                   KalmanUncertaintyAnglePitch = 4;
// // // // //                 Serial.println("AUTO DISARM");
// // // // //             }

// // // // //             Serial.println("TX DISCONNECTED");
// // // // //         }

// // // // //         lastRX = millis();
// // // // //     }


// // // // //        //         // ARM  khi giam xuong
// // // // //       if (connected && !armed && rx.ch[4] < 1050)
// // // // //       {
// // // // //           armed = true;

// // // // //           reset_pid();
// // // // //           KalmanAngleRoll = 0;
// // // // //           KalmanAnglePitch = 0;

// // // // //           KalmanUncertaintyAngleRoll = 4;
// // // // //           KalmanUncertaintyAnglePitch = 4;
// // // // //           Serial.println("ARM");

// // // // //       }

// // // // //       // DISARM
// // // // //       if (armed && rx.ch[4] > 1800)
// // // // //       {
// // // // //           armed = false;

// // // // //           reset_pid();
// // // // //           KalmanAngleRoll = 0;
// // // // //           KalmanAnglePitch = 0;

// // // // //           KalmanUncertaintyAngleRoll = 4;
// // // // //           KalmanUncertaintyAnglePitch = 4;
          
// // // // //           Serial.println("DISARM");
// // // // //       }


// // // // //       if(armed){

// // // // //             gyro_signals();
// // // // //             RateRoll-=RateCalibrationGyroRoll;
// // // // //             RatePitch-=RateCalibrationGyroPitch;
// // // // //             RateYaw-=RateCalibrationGyroYaw;

// // // // //             AccX-=RateCalibrationAccRoll;
// // // // //             AccY-=RateCalibrationAccPitch;
// // // // //             AccZ-=RateCalibrationAccYaw;


// // // // //           AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
// // // // //           AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);
            



// // // // //             //   Serial.print(" RateRoll:"); Serial.print(RateRoll);
// // // // //             // Serial.print(" RatePitch:"); Serial.print(RatePitch);
// // // // //             // Serial.print(" RateYaw:"); Serial.print(RateYaw);


// // // // //             // Serial.print(" AccX:"); Serial.print(AccX);
// // // // //             // Serial.print(" AccY:"); Serial.print(AccY);
// // // // //             // Serial.print(" AccZ:"); Serial.println(AccZ);
      


// // // // //             kalman_1d(KalmanAngleRoll, KalmanUncertaintyAngleRoll, RateRoll, AngleRoll);
// // // // //             KalmanAngleRoll=Kalman1DOutput[0]; KalmanUncertaintyAngleRoll=Kalman1DOutput[1];
// // // // //             kalman_1d(KalmanAnglePitch, KalmanUncertaintyAnglePitch, RatePitch, AnglePitch);
// // // // //             KalmanAnglePitch=Kalman1DOutput[0]; KalmanUncertaintyAnglePitch=Kalman1DOutput[1];
      

            
// // // // //             DesiredAngleRoll  = 0.10 * (rx.ch[3] - 1500);
// // // // //           DesiredAnglePitch = 0.10 * (rx.ch[2] - 1500);

  
// // // // //           // InputThrottle = map(rx.ch[0], 1000, 2000, 0, 255);
// // // // //           InputThrottle =(rx.ch[0]-1000)*255.0f/1000.0f;
// // // // //           InputThrottle = constrain(InputThrottle, 0, 255);

// // // // //           DesiredRateYaw = 0.08 * (rx.ch[1] - 1500);

             

// // // // //             ErrorAngleRoll=DesiredAngleRoll-KalmanAngleRoll;
// // // // //             ErrorAnglePitch=DesiredAnglePitch-KalmanAnglePitch;
// // // // //             pid_equation(ErrorAngleRoll, PAngleRoll, IAngleRoll, DAngleRoll, PrevErrorAngleRoll, PrevItermAngleRoll);     
// // // // //             DesiredRateRoll=PIDReturn[0]; 
// // // // //             PrevErrorAngleRoll=PIDReturn[1];
// // // // //             PrevItermAngleRoll=PIDReturn[2];
// // // // //             pid_equation(ErrorAnglePitch, PAnglePitch, IAnglePitch, DAnglePitch, PrevErrorAnglePitch, PrevItermAnglePitch);
// // // // //             DesiredRatePitch=PIDReturn[0]; 
// // // // //             PrevErrorAnglePitch=PIDReturn[1];
// // // // //             PrevItermAnglePitch=PIDReturn[2];
// // // // //             ErrorRateRoll=DesiredRateRoll-RateRoll;
// // // // //             ErrorRatePitch=DesiredRatePitch-RatePitch;
// // // // //             ErrorRateYaw=DesiredRateYaw-RateYaw;
// // // // //             pid_equation(ErrorRateRoll, PRateRoll, IRateRoll, DRateRoll, PrevErrorRateRoll, PrevItermRateRoll);
// // // // //                  InputRoll=PIDReturn[0];
// // // // //                  PrevErrorRateRoll=PIDReturn[1]; 
// // // // //                  PrevItermRateRoll=PIDReturn[2];
// // // // //             pid_equation(ErrorRatePitch, PRatePitch,IRatePitch, DRatePitch, PrevErrorRatePitch, PrevItermRatePitch);
// // // // //                  InputPitch=PIDReturn[0]; 
// // // // //                  PrevErrorRatePitch=PIDReturn[1]; 
// // // // //                  PrevItermRatePitch=PIDReturn[2];
// // // // //             pid_equation(ErrorRateYaw, PRateYaw,IRateYaw, DRateYaw, PrevErrorRateYaw, PrevItermRateYaw);
// // // // //                  InputYaw=PIDReturn[0]; 
// // // // //                  PrevErrorRateYaw=PIDReturn[1]; 
// // // // //                  PrevItermRateYaw=PIDReturn[2];

// // // // //             InputRoll  = constrain(InputRoll,  -60, 60);
// // // // //             InputPitch = constrain(InputPitch, -60, 60);
// // // // //             InputYaw   = constrain(InputYaw,   -40, 40);




// // // // //             // Serial.print(" InputRoll:"); Serial.print(InputRoll);
// // // // //             // Serial.print(" InputPitch:"); Serial.print(InputPitch);
// // // // //             // Serial.print(" InputYaw:"); Serial.println(InputYaw);
          

           
// // // // //           MotorInput1 = 1.024*(InputThrottle + InputRoll - InputPitch - InputYaw);
// // // // //           MotorInput2 = 1.024*(InputThrottle - InputRoll - InputPitch + InputYaw);
// // // // //           MotorInput3 = 1.024*(InputThrottle - InputRoll + InputPitch - InputYaw);
// // // // //           MotorInput4 = 1.024*(InputThrottle + InputRoll + InputPitch + InputYaw);


// // // // //           MotorInput1 = constrain(MotorInput1,0,255);
// // // // //           MotorInput2 = constrain(MotorInput2,0,255);
// // // // //           MotorInput3 = constrain(MotorInput3,0,255);
// // // // //           MotorInput4 = constrain(MotorInput4,0,255);

// // // // //           if(rx.ch[0] < 1050)
// // // // //           {
// // // // //               MotorInput1 = 0;
// // // // //               MotorInput2 = 0;
// // // // //               MotorInput3 = 0;
// // // // //               MotorInput4 = 0;
// // // // //               reset_pid();
// // // // //               PrevItermRateRoll=0;
// // // // //               PrevItermRatePitch=0;
// // // // //               PrevItermRateYaw=0;
// // // // //           }

// // // // //             analogWrite(M1, MotorInput1);
// // // // //             analogWrite(M2, MotorInput2);
// // // // //             analogWrite(M3, MotorInput3);
// // // // //             analogWrite(M4, MotorInput4);

// // // // //             //   ledcWrite(M1, MotorInput1);
// // // // //             // ledcWrite(M2, MotorInput2);
// // // // //             // ledcWrite(M3, MotorInput3);
// // // // //             // ledcWrite(M4, MotorInput4);


          
        

// // // // //             //   Serial.print(" M1:"); Serial.print(rx.ch[0]);
// // // // //             // Serial.print(" M2:"); Serial.print(rx.ch[1]);
// // // // //             // Serial.print(" M3:"); Serial.print(rx.ch[2]);
// // // // //             // Serial.print(" M4:"); Serial.print(rx.ch[3]);
// // // // //             // Serial.print(" M5:"); Serial.println(rx.ch[4]);
            


          

// // // // //             Serial.print(" M1:"); Serial.print(MotorInput1);
// // // // //             Serial.print(" M2:"); Serial.print(MotorInput2);
// // // // //             Serial.print(" M3:"); Serial.print(MotorInput3);
// // // // //             Serial.print(" M4:"); Serial.println(MotorInput4);


          
// // // // //       }

// // // // //       if(!armed)
// // // // //       {
// // // // //           reset_pid();

// // // // //           MotorInput1=0;
// // // // //           MotorInput2=0;
// // // // //           MotorInput3=0;
// // // // //           MotorInput4=0;

// // // // //           analogWrite(M1,0);
// // // // //           analogWrite(M2,0);
// // // // //           analogWrite(M3,0);
// // // // //           analogWrite(M4,0);


// // // // //           reset_pid();
// // // // //           KalmanAngleRoll = 0;
// // // // //           KalmanAnglePitch = 0;

// // // // //           KalmanUncertaintyAngleRoll = 4;
// // // // //           KalmanUncertaintyAnglePitch = 4;

// // // // //           // ledcWrite(CH1, 0);
// // // // //           // ledcWrite(CH2, 0);
// // // // //           // ledcWrite(CH3, 0);
// // // // //           // ledcWrite(CH4, 0);
// // // // //       }

// // // // //         while (micros() - LoopTimer < 4000);
// // // // //         LoopTimer=micros();

// // // // // }




















// // // // // #include <Arduino.h>
// // // // // #include <Wire.h>
// // // // // #include <WiFi.h>
// // // // // #include <esp_now.h>

// // // // // // Chân PWM cho từng motor
// // // // // int M1 = 3;
// // // // // int M2 = 6;
// // // // // int M3 = 5;
// // // // // int M4 = 4;


// // // // // float RateRoll, RatePitch, RateYaw;
// // // // // float RateCalibrationGyroRoll, RateCalibrationGyroPitch, RateCalibrationGyroYaw;


// // // // // float RateCalibrationAccRoll, RateCalibrationAccPitch, RateCalibrationAccYaw;
// // // // // int RateCalibrationNumber;
// // // // // float ReceiverValue[]={0, 0, 0, 0, 0, 0, 0, 0};
// // // // // int ChannelNumber=0; 
// // // // // float Voltage, Current, BatteryRemaining, BatteryAtStart;
// // // // // float CurrentConsumed=0;
// // // // // float BatteryDefault=1300;
// // // // // uint32_t LoopTimer;
// // // // // float DesiredRateRoll, DesiredRatePitch,DesiredRateYaw;
// // // // // float ErrorRateRoll, ErrorRatePitch, ErrorRateYaw;
// // // // // float InputRoll, InputThrottle, InputPitch, InputYaw;
// // // // // float PrevErrorRateRoll, PrevErrorRatePitch, PrevErrorRateYaw;
// // // // // float PrevItermRateRoll, PrevItermRatePitch, PrevItermRateYaw;
// // // // // float PIDReturn[]={0, 0, 0};
// // // // // // float PRateRoll= 1.14; float PRatePitch=PRateRoll; float PRateYaw=0;
// // // // // // float IRateRoll=0.05; float IRatePitch=IRateRoll; float IRateYaw=0.0;
// // // // // // float DRateRoll=0.03; float DRatePitch=DRateRoll; float DRateYaw=0.0;
// // // // // float PRateRoll= 1.14; float PRatePitch=PRateRoll; float PRateYaw=0;
// // // // // float IRateRoll=0.05; float IRatePitch=IRateRoll; float IRateYaw=0.12;
// // // // // float DRateRoll=0.03; float DRatePitch=DRateRoll; float DRateYaw=0.0;
// // // // // float MotorInput1, MotorInput2, MotorInput3, MotorInput4;
// // // // // float AccX, AccY, AccZ;
// // // // // float AngleRoll, AnglePitch;
// // // // // float KalmanAngleRoll=0, KalmanUncertaintyAngleRoll=2*2;
// // // // // float KalmanAnglePitch=0, KalmanUncertaintyAnglePitch=2*2;
// // // // // float Kalman1DOutput[]={0,0};
// // // // // float DesiredAngleRoll, DesiredAnglePitch;
// // // // // float ErrorAngleRoll, ErrorAnglePitch;
// // // // // float PrevErrorAngleRoll, PrevErrorAnglePitch;
// // // // // float PrevItermAngleRoll, PrevItermAnglePitch;
// // // // // float PAngleRoll=1.14; float PAnglePitch=PAngleRoll;
// // // // // float IAngleRoll=0.8; float IAnglePitch=IAngleRoll;
// // // // // float DAngleRoll=0.02; float DAnglePitch=DAngleRoll;

// // // // // int16_t AccXLSB;
// // // // // int16_t AccYLSB;
// // // // // int16_t AccZLSB;
// // // // // int16_t GyroX;
// // // // // int16_t GyroY;
// // // // // int16_t GyroZ;


// // // // // void kalman_1d(float KalmanState, float KalmanUncertainty, float KalmanInput, float KalmanMeasurement) {
// // // // //   KalmanState=KalmanState+0.004*KalmanInput;
// // // // //   KalmanUncertainty=KalmanUncertainty + 0.004 * 0.004 * 4 * 4;
// // // // //   float KalmanGain=KalmanUncertainty * 1/(1*KalmanUncertainty + 3 * 3);
// // // // //   KalmanState=KalmanState+KalmanGain * (KalmanMeasurement-KalmanState);
// // // // //   KalmanUncertainty=(1-KalmanGain) * KalmanUncertainty;
// // // // //   Kalman1DOutput[0]=KalmanState; 
// // // // //   Kalman1DOutput[1]=KalmanUncertainty;
// // // // // }

// // // // // // ================= DATA =================
// // // // // typedef struct {
// // // // //   uint16_t ch[8];
// // // // // } Data;

// // // // // Data rx;

// // // // // // ================= STATE =================
// // // // // unsigned long lastRX = 0;

// // // // // bool connected = false;
// // // // // bool armed = false;

// // // // // uint8_t lostCount = 0;

// // // // // // ================= ESP-NOW CALLBACK =================
// // // // // void onRecv(const uint8_t *mac, const uint8_t *data, int len)
// // // // // {
// // // // //     memcpy(&rx, data, sizeof(rx));

// // // // //     lastRX = millis();
// // // // //     connected = true;
// // // // //     lostCount = 0;
// // // // // }

// // // // // void gyro_signals(void) {
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1A);
// // // // //   Wire.write(0x05);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1C);
// // // // //   Wire.write(0x10);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x3B);
// // // // //   Wire.endTransmission(); 
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   // int16_t AccXLSB = Wire.read() << 8 | Wire.read();
// // // // //   // int16_t AccYLSB = Wire.read() << 8 | Wire.read();
// // // // //   // int16_t AccZLSB = Wire.read() << 8 | Wire.read();

// // // // //    AccXLSB = Wire.read() << 8 | Wire.read();
// // // // //    AccYLSB = Wire.read() << 8 | Wire.read();
// // // // //    AccZLSB = Wire.read() << 8 | Wire.read();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1B); 
// // // // //   Wire.write(0x8);
// // // // //   Wire.endTransmission();                                                   
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x43);
// // // // //   Wire.endTransmission();
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   // int16_t GyroX=Wire.read()<<8 | Wire.read();
// // // // //   // int16_t GyroY=Wire.read()<<8 | Wire.read();
// // // // //   // int16_t GyroZ=Wire.read()<<8 | Wire.read();


// // // // //    GyroX=Wire.read()<<8 | Wire.read();
// // // // //    GyroY=Wire.read()<<8 | Wire.read();
// // // // //    GyroZ=Wire.read()<<8 | Wire.read();
// // // // //   RateRoll=(float)GyroX/65.5;
// // // // //   RatePitch=(float)GyroY/65.5;
// // // // //   RateYaw=(float)GyroZ/65.5;
// // // // //   AccX=(float)AccXLSB/4096;
// // // // //   AccY=(float)AccYLSB/4096;
// // // // //   AccZ=(float)AccZLSB/4096;
// // // // //   // AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
// // // // //   // AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);
// // // // // }
// // // // // void pid_equation(float Error, float P , float I, float D, float PrevError, float PrevIterm) {
// // // // //   float Pterm=P*Error;
// // // // //   float Iterm=PrevIterm+I*(Error+PrevError)*0.004/2;
// // // // //   // if (Iterm > 400) Iterm=400;
// // // // //   // else if (Iterm <-400) Iterm=-400;
// // // // //     if(Iterm>100) Iterm=100;
// // // // //     if(Iterm<-100) Iterm=-100;

// // // // //   float Dterm=D*(Error-PrevError)/0.004;
// // // // //   float PIDOutput= Pterm+Iterm+Dterm;
// // // // //   // if (PIDOutput>400) PIDOutput=400;
// // // // //   // else if (PIDOutput <-400) PIDOutput=-400;
// // // // //     if(PIDOutput > 100) PIDOutput = 100;
// // // // //     if(PIDOutput < -100) PIDOutput = -100;
// // // // //   PIDReturn[0]=PIDOutput;
// // // // //   PIDReturn[1]=Error;
// // // // //   PIDReturn[2]=Iterm;
// // // // // }



// // // // // void reset_pid(void) {
// // // // //   PrevErrorRateRoll=0; PrevErrorRatePitch=0; PrevErrorRateYaw=0;
// // // // //   PrevItermRateRoll=0; PrevItermRatePitch=0; PrevItermRateYaw=0;
// // // // //   PrevErrorAngleRoll=0; PrevErrorAnglePitch=0;    
// // // // //   PrevItermAngleRoll=0; PrevItermAnglePitch=0;
// // // // // }
// // // // // void setup() {

// // // // //     Serial.begin(115200);
// // // // //     pinMode(M1, OUTPUT);
// // // // //     pinMode(M2, OUTPUT);
// // // // //     pinMode(M3, OUTPUT);
// // // // //     pinMode(M4, OUTPUT);

// // // // // //    ledcSetup(CH1, 400, 8);
// // // // // // ledcSetup(CH2, 400, 8);
// // // // // // ledcSetup(CH3, 400, 8);
// // // // // // ledcSetup(CH4, 400, 8);

// // // // // // ledcAttachPin(M1, CH1);
// // // // // // ledcAttachPin(M2, CH2);
// // // // // // ledcAttachPin(M3, CH3);
// // // // // // ledcAttachPin(M4, CH4);

// // // // //     WiFi.mode(WIFI_STA);

// // // // //     if (esp_now_init() != ESP_OK)
// // // // //     {
// // // // //         Serial.println("ESP-NOW FAIL");
// // // // //         return;
// // // // //     }

// // // // //     esp_now_register_recv_cb(onRecv);

// // // // //     Serial.println("RX READY");

// // // // //   Wire.setClock(400000);
// // // // //   Wire.begin();
// // // // //   delay(250);
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x6B);
// // // // //   Wire.write(0x00);
// // // // //   Wire.endTransmission();
// // // // //   for (RateCalibrationNumber=0; 
// // // // //         RateCalibrationNumber<2000;
// // // // //         RateCalibrationNumber ++) {
// // // // //           gyro_signals();
// // // // //     RateCalibrationGyroRoll+=RateRoll;
// // // // //     RateCalibrationGyroPitch+=RatePitch;
// // // // //     RateCalibrationGyroYaw+=RateYaw;


// // // // //     RateCalibrationAccRoll+=AccX;
// // // // //     RateCalibrationAccPitch+=AccY;
// // // // //     RateCalibrationAccYaw+=AccZ;
// // // // //     delay(1);
// // // // //   }
// // // // //   RateCalibrationGyroRoll/=2000;
// // // // //   RateCalibrationGyroPitch/=2000;
// // // // //   RateCalibrationGyroYaw/=2000;


// // // // //   // RateCalibrationAccRoll/=2000;
// // // // //   // RateCalibrationAccPitch/=2000;
// // // // //   // RateCalibrationAccYaw/=2000;

// // // // //     float avgX = RateCalibrationAccRoll / 2000;
// // // // //     float avgY = RateCalibrationAccPitch / 2000;
// // // // //     float avgZ =   RateCalibrationAccYaw/=2000;


// // // // // //     // Vì đang đặt nằm ngang:
// // // // // //     // X = 0g
// // // // // //     // Y = 0g
// // // // // //     // Z = +1g = 16384

// // // // //   RateCalibrationAccRoll = avgX;
// // // // // RateCalibrationAccPitch = avgY;
// // // // // RateCalibrationAccYaw = avgZ - 1;


// // // // //   LoopTimer=micros();
// // // // // }

// // // // // void loop() {


// // // // //    // ================= CHECK LOST TX =================
// // // // //     if (connected && millis() - lastRX > 300)
// // // // //     {
// // // // //         lostCount++;

// // // // //         if (lostCount >= 5)
// // // // //         {
// // // // //             connected = false;

// // // // //             if (armed)
// // // // //             {
// // // // //                 armed = false;
// // // // //                  reset_pid();
// // // // //                  KalmanAngleRoll = 0;
// // // // //                   KalmanAnglePitch = 0;

// // // // //                   KalmanUncertaintyAngleRoll = 4;
// // // // //                   KalmanUncertaintyAnglePitch = 4;
// // // // //                 Serial.println("AUTO DISARM");
// // // // //             }

// // // // //             Serial.println("TX DISCONNECTED");
// // // // //         }

// // // // //         lastRX = millis();
// // // // //     }


// // // // //        //         // ARM  khi giam xuong
// // // // //       if (connected && !armed && rx.ch[4] < 1050)
// // // // //       {
// // // // //           armed = true;
// // // // //           Serial.println("ARM");

// // // // //       }

// // // // //       // DISARM
// // // // //       if (armed && rx.ch[4] > 1800)
// // // // //       {
// // // // //           armed = false;
          
// // // // //           Serial.println("DISARM");
// // // // //       }


// // // // //       if(armed){

// // // // //             gyro_signals();
// // // // //             RateRoll-=RateCalibrationGyroRoll;
// // // // //             RatePitch-=RateCalibrationGyroPitch;
// // // // //             RateYaw-=RateCalibrationGyroYaw;

// // // // //             AccX-=RateCalibrationAccRoll;
// // // // //             AccY-=RateCalibrationAccPitch;
// // // // //             AccZ-=RateCalibrationAccYaw;


// // // // //           AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
// // // // //           AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);
            



// // // // //             //   Serial.print(" RateRoll:"); Serial.print(RateRoll);
// // // // //             // Serial.print(" RatePitch:"); Serial.print(RatePitch);
// // // // //             // Serial.print(" RateYaw:"); Serial.print(RateYaw);


// // // // //             // Serial.print(" AccX:"); Serial.print(AccX);
// // // // //             // Serial.print(" AccY:"); Serial.print(AccY);
// // // // //             // Serial.print(" AccZ:"); Serial.println(AccZ);
      


// // // // //             kalman_1d(KalmanAngleRoll, KalmanUncertaintyAngleRoll, RateRoll, AngleRoll);
// // // // //             KalmanAngleRoll=Kalman1DOutput[0]; KalmanUncertaintyAngleRoll=Kalman1DOutput[1];
// // // // //             kalman_1d(KalmanAnglePitch, KalmanUncertaintyAnglePitch, RatePitch, AnglePitch);
// // // // //             KalmanAnglePitch=Kalman1DOutput[0]; KalmanUncertaintyAnglePitch=Kalman1DOutput[1];
      

            
// // // // //             DesiredAngleRoll  = 0.10 * (rx.ch[3] - 1500);
// // // // //           DesiredAnglePitch = 0.10 * (rx.ch[2] - 1500);

       

// // // // //           // InputThrottle = map(rx.ch[0], 1000, 2000, 0, 255);
// // // // //           InputThrottle =(rx.ch[0]-1000)*255.0f/1000.0f;
// // // // //           InputThrottle = constrain(InputThrottle, 0, 255);

// // // // //           DesiredRateYaw = 0.15 * (rx.ch[1] - 1500);

// // // // //             ErrorAngleRoll=DesiredAngleRoll-KalmanAngleRoll;
// // // // //             ErrorAnglePitch=DesiredAnglePitch-KalmanAnglePitch;
// // // // //             pid_equation(ErrorAngleRoll, PAngleRoll, IAngleRoll, DAngleRoll, PrevErrorAngleRoll, PrevItermAngleRoll);     
// // // // //             DesiredRateRoll=PIDReturn[0]; 
// // // // //             PrevErrorAngleRoll=PIDReturn[1];
// // // // //             PrevItermAngleRoll=PIDReturn[2];
// // // // //             pid_equation(ErrorAnglePitch, PAnglePitch, IAnglePitch, DAnglePitch, PrevErrorAnglePitch, PrevItermAnglePitch);
// // // // //             DesiredRatePitch=PIDReturn[0]; 
// // // // //             PrevErrorAnglePitch=PIDReturn[1];
// // // // //             PrevItermAnglePitch=PIDReturn[2];
// // // // //             ErrorRateRoll=DesiredRateRoll-RateRoll;
// // // // //             ErrorRatePitch=DesiredRatePitch-RatePitch;
// // // // //             ErrorRateYaw=DesiredRateYaw-RateYaw;
// // // // //             pid_equation(ErrorRateRoll, PRateRoll, IRateRoll, DRateRoll, PrevErrorRateRoll, PrevItermRateRoll);
// // // // //                  InputRoll=PIDReturn[0];
// // // // //                  PrevErrorRateRoll=PIDReturn[1]; 
// // // // //                  PrevItermRateRoll=PIDReturn[2];
// // // // //             pid_equation(ErrorRatePitch, PRatePitch,IRatePitch, DRatePitch, PrevErrorRatePitch, PrevItermRatePitch);
// // // // //                  InputPitch=PIDReturn[0]; 
// // // // //                  PrevErrorRatePitch=PIDReturn[1]; 
// // // // //                  PrevItermRatePitch=PIDReturn[2];
// // // // //             pid_equation(ErrorRateYaw, PRateYaw,IRateYaw, DRateYaw, PrevErrorRateYaw, PrevItermRateYaw);
// // // // //                  InputYaw=PIDReturn[0]; 
// // // // //                  PrevErrorRateYaw=PIDReturn[1]; 
// // // // //                  PrevItermRateYaw=PIDReturn[2];

// // // // //             InputRoll  = constrain(InputRoll,  -60, 60);
// // // // //             InputPitch = constrain(InputPitch, -60, 60);
// // // // //             InputYaw   = constrain(InputYaw,   -40, 40);


// // // // //             // Serial.print(" InputRoll:"); Serial.print(InputRoll);
// // // // //             // Serial.print(" InputPitch:"); Serial.print(InputPitch);
// // // // //             // Serial.print(" InputYaw:"); Serial.println(InputYaw);
          

           
// // // // //           MotorInput1 = 1.024*(InputThrottle + InputRoll - InputPitch - InputYaw);
// // // // //           MotorInput2 = 1.024*(InputThrottle - InputRoll - InputPitch + InputYaw);
// // // // //           MotorInput3 = 1.024*(InputThrottle - InputRoll + InputPitch - InputYaw);
// // // // //           MotorInput4 = 1.024*(InputThrottle + InputRoll + InputPitch + InputYaw);


// // // // //           MotorInput1 = constrain(MotorInput1,0,255);
// // // // //           MotorInput2 = constrain(MotorInput2,0,255);
// // // // //           MotorInput3 = constrain(MotorInput3,0,255);
// // // // //           MotorInput4 = constrain(MotorInput4,0,255);

// // // // //           if(rx.ch[0] < 1050)
// // // // //           {
// // // // //               MotorInput1 = 0;
// // // // //               MotorInput2 = 0;
// // // // //               MotorInput3 = 0;
// // // // //               MotorInput4 = 0;
// // // // //               reset_pid();
// // // // //           }

// // // // //             analogWrite(M1, MotorInput1);
// // // // //             analogWrite(M2, MotorInput2);
// // // // //             analogWrite(M3, MotorInput3);
// // // // //             analogWrite(M4, MotorInput4);

// // // // //             //   ledcWrite(M1, MotorInput1);
// // // // //             // ledcWrite(M2, MotorInput2);
// // // // //             // ledcWrite(M3, MotorInput3);
// // // // //             // ledcWrite(M4, MotorInput4);


          
        

// // // // //             //   Serial.print(" M1:"); Serial.print(rx.ch[0]);
// // // // //             // Serial.print(" M2:"); Serial.print(rx.ch[1]);
// // // // //             // Serial.print(" M3:"); Serial.print(rx.ch[2]);
// // // // //             // Serial.print(" M4:"); Serial.print(rx.ch[3]);
// // // // //             // Serial.print(" M5:"); Serial.println(rx.ch[4]);
            


          

// // // // //             Serial.print(" M1:"); Serial.print(MotorInput1);
// // // // //             Serial.print(" M2:"); Serial.print(MotorInput2);
// // // // //             Serial.print(" M3:"); Serial.print(MotorInput3);
// // // // //             Serial.print(" M4:"); Serial.println(MotorInput4);


          
// // // // //       }

// // // // //       if(!armed)
// // // // //       {
// // // // //           reset_pid();

// // // // //           MotorInput1=0;
// // // // //           MotorInput2=0;
// // // // //           MotorInput3=0;
// // // // //           MotorInput4=0;

// // // // //           analogWrite(M1,0);
// // // // //           analogWrite(M2,0);
// // // // //           analogWrite(M3,0);
// // // // //           analogWrite(M4,0);

// // // // //           // ledcWrite(CH1, 0);
// // // // //           // ledcWrite(CH2, 0);
// // // // //           // ledcWrite(CH3, 0);
// // // // //           // ledcWrite(CH4, 0);
// // // // //       }

// // // // //         while (micros() - LoopTimer < 4000);
// // // // //         LoopTimer=micros();

// // // // // }




















// // // // // /*
// // // // // ---------------------------------------------------
// // // // // PART 2
// // // // // GYROSCOPE CALIBRATION
// // // // // ESP32-C3 + MPU6050
// // // // // ---------------------------------------------------
// // // // // */
// // // // //   #include <Arduino.h>
// // // // // #include <Wire.h>

// // // // // #define MPU_ADDR 0x68

// // // // // #define SDA_PIN 21
// // // // // #define SCL_PIN 22

// // // // // int16_t AccX, AccY, AccZ;
// // // // // int16_t GyroX, GyroY, GyroZ;
// // // // // int16_t Temp;


// // // // // float AccRateX = 0;
// // // // // float AccRateY = 0;
// // // // // float AccRateZ = 0;


// // // // // float AccOffsetX = 0;
// // // // // float AccOffsetY = 0;
// // // // // float AccOffsetZ = 0;


// // // // // float GyroRateX;
// // // // // float GyroRateY;
// // // // // float GyroRateZ;

// // // // // float GyroOffsetX = 0;
// // // // // float GyroOffsetY = 0;
// // // // // float GyroOffsetZ = 0;


// // // // // //====================================================
// // // // // // Góc từ Accelerometer
// // // // // //====================================================
// // // // // float RollAcc = 0;
// // // // // float PitchAcc = 0;

// // // // // //====================================================
// // // // // // Kalman
// // // // // //====================================================
// // // // // float KalmanRoll = 0;
// // // // // float KalmanPitch = 0;

// // // // // float KalmanRollUncertainty = 4;
// // // // // float KalmanPitchUncertainty = 4;

// // // // // float Kalman1DOutput[2];

// // // // // ///sdsa

// // // // // //--------------------------------------------------
// // // // // // Desired Angle
// // // // // //--------------------------------------------------
// // // // // float DesiredAngleRoll = 0;
// // // // // float DesiredAnglePitch = 0;

// // // // // //--------------------------------------------------
// // // // // // Desired Rate
// // // // // //--------------------------------------------------
// // // // // float DesiredRateRoll = 0;
// // // // // float DesiredRatePitch = 0;

// // // // // //--------------------------------------------------
// // // // // // PID Angle
// // // // // //--------------------------------------------------
// // // // // float ErrorAngleRoll = 0;
// // // // // float ErrorAnglePitch = 0;

// // // // // float PrevErrorAngleRoll = 0;
// // // // // float PrevErrorAnglePitch = 0;

// // // // // float PrevItermAngleRoll = 0;
// // // // // float PrevItermAnglePitch = 0;

// // // // // //--------------------------------------------------
// // // // // // PID Gain
// // // // // //--------------------------------------------------
// // // // // float PAngleRoll = 1.14f;
// // // // // float IAngleRoll = 0.05f;
// // // // // float DAngleRoll = 0.03f;

// // // // // float PAnglePitch = PAngleRoll;
// // // // // float IAnglePitch = IAngleRoll;
// // // // // float DAnglePitch = DAngleRoll;

// // // // // float PIDReturn[3];



// // // // // void write8(uint8_t reg, uint8_t value)
// // // // // {
// // // // //     Wire.beginTransmission(MPU_ADDR);
// // // // //     Wire.write(reg);
// // // // //     Wire.write(value);
// // // // //     Wire.endTransmission(true);
// // // // // }

// // // // // void initMPU()
// // // // // {
// // // // //     write8(0x6B,0x00);     // Wake up
// // // // //     write8(0x1B,0x00);     // Gyro ±250dps
// // // // //     write8(0x1C,0x00);     // Acc ±2g

// // // // //     delay(100);
// // // // // }

// // // // // void readMPU()
// // // // // {
// // // // //     Wire.beginTransmission(MPU_ADDR);
// // // // //     Wire.write(0x3B);
// // // // //     Wire.endTransmission(false);

// // // // //     Wire.requestFrom(MPU_ADDR,14,true);

// // // // //     AccX=(Wire.read()<<8)|Wire.read();
// // // // //     AccY=(Wire.read()<<8)|Wire.read();
// // // // //     AccZ=(Wire.read()<<8)|Wire.read();

// // // // //     Temp=(Wire.read()<<8)|Wire.read();

// // // // //     GyroX=(Wire.read()<<8)|Wire.read();
// // // // //     GyroY=(Wire.read()<<8)|Wire.read();
// // // // //     GyroZ=(Wire.read()<<8)|Wire.read();
// // // // // }

// // // // // void calibrateGyro_Acc()
// // // // // {
// // // // //     long sumGyroX=0;
// // // // //     long sumGyroY=0;
// // // // //     long sumGyroZ=0;


// // // // //     long sumAccX=0;
// // // // //     long sumAccY=0;
// // // // //     long sumAccZ=0;

// // // // //     Serial.println();
// // // // //     Serial.println("================================");
// // // // //     Serial.println("DO NOT MOVE MPU6050");
// // // // //     Serial.println("Calibrating...");
// // // // //     Serial.println("================================");

// // // // //     delay(2000);

// // // // //     for(int i=0;i<2000;i++)
// // // // //     {
// // // // //         readMPU();

// // // // //         sumGyroX+=GyroX;
// // // // //         sumGyroY+=GyroY;
// // // // //         sumGyroZ+=GyroZ;

// // // // //         sumAccX += AccX;
// // // // //         sumAccY += AccY;
// // // // //         sumAccZ += AccZ;

// // // // //         if(i%200==0)
// // // // //             Serial.print(".");

// // // // //         delay(2);
// // // // //     }

// // // // //     GyroOffsetX=(float)sumGyroX/2000.0f;
// // // // //     GyroOffsetY=(float)sumGyroY/2000.0f;
// // // // //     GyroOffsetZ=(float)sumGyroZ/2000.0f;


// // // // //     float avgX = sumAccX / 2000.0f;
// // // // //     float avgY = sumAccY / 2000.0f;
// // // // //     float avgZ = sumAccZ / 2000.0f;

// // // // //     // Vì đang đặt nằm ngang:
// // // // //     // X = 0g
// // // // //     // Y = 0g
// // // // //     // Z = +1g = 16384

// // // // //     AccOffsetX = avgX;
// // // // //     AccOffsetY = avgY;
// // // // //     AccOffsetZ = avgZ - 16384.0f;

// // // // //     // Serial.println();
// // // // //     // Serial.println();

// // // // //     // Serial.print("Gyro Offset X = ");
// // // // //     // Serial.println(GyroOffsetX);

// // // // //     // Serial.print("Gyro Offset Y = ");
// // // // //     // Serial.println(GyroOffsetY);

// // // // //     // Serial.print("Gyro Offset Z = ");
// // // // //     // Serial.println(GyroOffsetZ);

// // // // //     // Serial.println();
// // // // // }

// // // // // void kalman_1d(
// // // // //     float KalmanState,
// // // // //     float KalmanUncertainty,
// // // // //     float KalmanInput,
// // // // //     float KalmanMeasurement)
// // // // // {
// // // // //     KalmanState =
// // // // //         KalmanState + 0.004f * KalmanInput;

// // // // //     KalmanUncertainty =
// // // // //         KalmanUncertainty +
// // // // //         0.004f * 0.004f * 4 * 4;

// // // // //     float KalmanGain =
// // // // //         KalmanUncertainty /
// // // // //         (KalmanUncertainty + 9);

// // // // //     KalmanState =
// // // // //         KalmanState +
// // // // //         KalmanGain *
// // // // //         (KalmanMeasurement - KalmanState);

// // // // //     KalmanUncertainty =
// // // // //         (1 - KalmanGain) *
// // // // //         KalmanUncertainty;

// // // // //     Kalman1DOutput[0] = KalmanState;
// // // // //     Kalman1DOutput[1] = KalmanUncertainty;
// // // // // }


// // // // // void pid_equation(
// // // // //     float Error,
// // // // //     float P,
// // // // //     float I,
// // // // //     float D,
// // // // //     float PrevError,
// // // // //     float PrevIterm)
// // // // // {
// // // // //     float Pterm = P * Error;

// // // // //     float Iterm =
// // // // //         PrevIterm +
// // // // //         I * (Error + PrevError) * 0.02f / 2.0f;

// // // // //     if(Iterm > 400) Iterm = 400;
// // // // //     if(Iterm < -400) Iterm = -400;

// // // // //     float Dterm =
// // // // //         D *
// // // // //         (Error - PrevError) /
// // // // //         0.02f;

// // // // //     float Output =
// // // // //         Pterm +
// // // // //         Iterm +
// // // // //         Dterm;

// // // // //     if(Output > 400) Output = 400;
// // // // //     if(Output < -400) Output = -400;

// // // // //     PIDReturn[0] = Output;
// // // // //     PIDReturn[1] = Error;
// // // // //     PIDReturn[2] = Iterm;
// // // // // }

// // // // // void setup()
// // // // // {
// // // // //     Serial.begin(115200);

// // // // //     Wire.begin(SDA_PIN,SCL_PIN);
// // // // //     Wire.setClock(400000);

// // // // //     delay(500);

// // // // //     initMPU();

// // // // //     calibrateGyro_Acc();
// // // // //     // calibrateAccel();
// // // // // }

// // // // // void loop()
// // // // // {
// // // // //     readMPU();

// // // // //     GyroRateX=(GyroX-GyroOffsetX)/131.0f;
// // // // //     GyroRateY=(GyroY-GyroOffsetY)/131.0f;
// // // // //     GyroRateZ=(GyroZ-GyroOffsetZ)/131.0f;


// // // // //     AccRateX = (AccX - AccOffsetX) / 16384.0f;
// // // // //     AccRateY = (AccY - AccOffsetY) / 16384.0f;
// // // // //     AccRateZ = (AccZ - AccOffsetZ) / 16384.0f;


// // // // //   //--------------------------------------------------
// // // // // // Góc từ Accelerometer
// // // // // //--------------------------------------------------
// // // // //     RollAcc =
// // // // //     atan2(
// // // // //         AccRateY,
// // // // //         AccRateZ
// // // // //     ) * 57.2957795f;

// // // // //     PitchAcc =
// // // // //     atan2(
// // // // //         -AccRateX,
// // // // //         sqrt(
// // // // //             AccRateY * AccRateY +
// // // // //             AccRateZ * AccRateZ
// // // // //         )
// // // // //     ) * 57.2957795f;


// // // // //    //--------------------------------------------------
// // // // //     // Kalman Roll
// // // // //     //--------------------------------------------------
// // // // //     kalman_1d(
// // // // //         KalmanRoll,
// // // // //         KalmanRollUncertainty,
// // // // //         GyroRateX,
// // // // //         RollAcc
// // // // //     );

// // // // //     KalmanRoll = Kalman1DOutput[0];
// // // // //     KalmanRollUncertainty = Kalman1DOutput[1];

// // // // //     //--------------------------------------------------
// // // // //     // Kalman Pitch
// // // // //     //--------------------------------------------------
// // // // //     kalman_1d(
// // // // //         KalmanPitch,
// // // // //         KalmanPitchUncertainty,
// // // // //         GyroRateY,
// // // // //         PitchAcc
// // // // //     );

// // // // //     KalmanPitch = Kalman1DOutput[0];
// // // // //     KalmanPitchUncertainty = Kalman1DOutput[1];

// // // // //     //--------------------------------------------------
// // // // // // Drone muốn giữ cân bằng
// // // // // //--------------------------------------------------
// // // // // DesiredAngleRoll = 0;
// // // // // DesiredAnglePitch = 0;


// // // // // ErrorAngleRoll =
// // // // // DesiredAngleRoll -
// // // // // KalmanRoll;

// // // // // ErrorAnglePitch =
// // // // // DesiredAnglePitch -
// // // // // KalmanPitch;


// // // // // pid_equation(
// // // // //     ErrorAngleRoll,
// // // // //     PAngleRoll,
// // // // //     IAngleRoll,
// // // // //     DAngleRoll,
// // // // //     PrevErrorAngleRoll,
// // // // //     PrevItermAngleRoll
// // // // // );

// // // // // DesiredRateRoll = PIDReturn[0];

// // // // // PrevErrorAngleRoll = PIDReturn[1];
// // // // // PrevItermAngleRoll = PIDReturn[2];


// // // // // pid_equation(
// // // // //     ErrorAnglePitch,
// // // // //     PAnglePitch,
// // // // //     IAnglePitch,
// // // // //     DAnglePitch,
// // // // //     PrevErrorAnglePitch,
// // // // //     PrevItermAnglePitch
// // // // // );

// // // // // DesiredRatePitch = PIDReturn[0];

// // // // // PrevErrorAnglePitch = PIDReturn[1];
// // // // // PrevItermAnglePitch = PIDReturn[2];


// // // // // Serial.print("Roll=");
// // // // // Serial.print(KalmanRoll,2);

// // // // // Serial.print("  DesiredRate=");
// // // // // Serial.print(DesiredRateRoll,2);

// // // // // Serial.print("  Pitch=");
// // // // // Serial.print(KalmanPitch,2);

// // // // // Serial.print("  DesiredRatePitch=");
// // // // // Serial.println(DesiredRatePitch,2);
    
// // // // // //   Serial.print("RollAcc = ");
// // // // // // Serial.print(RollAcc,2);

// // // // // // Serial.print("  PitchAcc = ");
// // // // // // Serial.print(PitchAcc,2);

// // // // // // Serial.print("  KalmanRoll = ");
// // // // // // Serial.print(KalmanRoll,2);

// // // // // // Serial.print("  KalmanPitch = ");
// // // // // // Serial.println(KalmanPitch,2);

// // // // // //     Serial.print("RollAcc = ");
// // // // // // Serial.print(RollAcc,2);

// // // // // // Serial.print("  PitchAcc = ");
// // // // // // Serial.print(PitchAcc,2);

// // // // // // Serial.print("  KalmanRoll = ");
// // // // // // Serial.print(KalmanRoll,2);

// // // // // // Serial.print("  KalmanPitch = ");
// // // // // // Serial.println(KalmanPitch,2);

// // // // // //     Serial.print("GyroRateX = ");
// // // // // // Serial.print(GyroRateX,3);

// // // // // // Serial.print(" GyroRateY = ");
// // // // // // Serial.print(GyroRateY,3);

// // // // // // Serial.print(" GyroRateZ = ");
// // // // // // Serial.print(GyroRateZ,3);


// // // // // //     Serial.print("AccRateX = ");
// // // // // // Serial.print(AccRateX,3);

// // // // // // Serial.print(" AccRateY = ");
// // // // // // Serial.print(AccRateY,3);

// // // // // // Serial.print(" AccRateZ = ");
// // // // // // Serial.println(AccRateZ,3);

// // // // //     // Serial.print("Gyro X : ");
// // // // //     // Serial.print(GyroRateX,3);

// // // // //     // Serial.print(" deg/s   ");

// // // // //     // Serial.print("Gyro Y : ");
// // // // //     // Serial.print(GyroRateY,3);

// // // // //     // Serial.print(" deg/s   ");

// // // // //     // Serial.print("Gyro Z : ");
// // // // //     // Serial.print(GyroRateZ,3);

// // // // //     // Serial.println(" deg/s");

// // // // //     delay(20);
// // // // // }















// // // // // /*
// // // // // ---------------------------------------------------
// // // // // PART 2
// // // // // GYROSCOPE CALIBRATION
// // // // // ESP32-C3 + MPU6050
// // // // // ---------------------------------------------------
// // // // // */
// // // // //   #include <Arduino.h>
// // // // // #include <Wire.h>

// // // // // #define MPU_ADDR 0x68

// // // // // #define SDA_PIN 21
// // // // // #define SCL_PIN 22

// // // // // int16_t AccX, AccY, AccZ;
// // // // // int16_t GyroX, GyroY, GyroZ;
// // // // // int16_t Temp;

// // // // // float GyroOffsetX = 0;
// // // // // float GyroOffsetY = 0;
// // // // // float GyroOffsetZ = 0;

// // // // // float GyroRateX;
// // // // // float GyroRateY;
// // // // // float GyroRateZ;




// // // // // float AccRateX = 0;
// // // // // float AccRateY = 0;
// // // // // float AccRateZ = 0;


// // // // // float AccOffsetX = 0;
// // // // // float AccOffsetY = 0;
// // // // // float AccOffsetZ = 0;


// // // // // void write8(uint8_t reg, uint8_t value)
// // // // // {
// // // // //     Wire.beginTransmission(MPU_ADDR);
// // // // //     Wire.write(reg);
// // // // //     Wire.write(value);
// // // // //     Wire.endTransmission(true);
// // // // // }

// // // // // void initMPU()
// // // // // {
// // // // //     write8(0x6B,0x00);     // Wake up
// // // // //     write8(0x1B,0x00);     // Gyro ±250dps
// // // // //     write8(0x1C,0x00);     // Acc ±2g

// // // // //     delay(100);
// // // // // }

// // // // // void readMPU()
// // // // // {
// // // // //     Wire.beginTransmission(MPU_ADDR);
// // // // //     Wire.write(0x3B);
// // // // //     Wire.endTransmission(false);

// // // // //     Wire.requestFrom(MPU_ADDR,14,true);

// // // // //     AccX=(Wire.read()<<8)|Wire.read();
// // // // //     AccY=(Wire.read()<<8)|Wire.read();
// // // // //     AccZ=(Wire.read()<<8)|Wire.read();

// // // // //     Temp=(Wire.read()<<8)|Wire.read();

// // // // //     GyroX=(Wire.read()<<8)|Wire.read();
// // // // //     GyroY=(Wire.read()<<8)|Wire.read();
// // // // //     GyroZ=(Wire.read()<<8)|Wire.read();
// // // // // }

// // // // // void calibrateGyro()
// // // // // {
// // // // //     long sumGyroX=0;
// // // // //     long sumGyroY=0;
// // // // //     long sumGyroZ=0;


// // // // //     long sumAccX=0;
// // // // //     long sumAccY=0;
// // // // //     long sumAccZ=0;

// // // // //     Serial.println();
// // // // //     Serial.println("================================");
// // // // //     Serial.println("DO NOT MOVE MPU6050");
// // // // //     Serial.println("Calibrating...");
// // // // //     Serial.println("================================");

// // // // //     delay(2000);

// // // // //     for(int i=0;i<2000;i++)
// // // // //     {
// // // // //         readMPU();

// // // // //         sumGyroX+=GyroX;
// // // // //         sumGyroY+=GyroY;
// // // // //         sumGyroZ+=GyroZ;

// // // // //         sumAccX += AccX;
// // // // //         sumAccY += AccY;
// // // // //         sumAccZ += AccZ;

// // // // //         if(i%200==0)
// // // // //             Serial.print(".");

// // // // //         delay(2);
// // // // //     }

// // // // //     GyroOffsetX=(float)sumGyroX/2000.0f;
// // // // //     GyroOffsetY=(float)sumGyroY/2000.0f;
// // // // //     GyroOffsetZ=(float)sumGyroZ/2000.0f;


// // // // //     float avgX = sumAccX / 2000.0f;
// // // // //     float avgY = sumAccY / 2000.0f;
// // // // //     float avgZ = sumAccZ / 2000.0f;

// // // // //     // Vì đang đặt nằm ngang:
// // // // //     // X = 0g
// // // // //     // Y = 0g
// // // // //     // Z = +1g = 16384

// // // // //     AccOffsetX = avgX;
// // // // //     AccOffsetY = avgY;
// // // // //     AccOffsetZ = avgZ - 16384.0f;

// // // // //     Serial.println();
// // // // //     Serial.println();

// // // // //     Serial.print("Gyro Offset X = ");
// // // // //     Serial.println(GyroOffsetX);

// // // // //     Serial.print("Gyro Offset Y = ");
// // // // //     Serial.println(GyroOffsetY);

// // // // //     Serial.print("Gyro Offset Z = ");
// // // // //     Serial.println(GyroOffsetZ);

// // // // //     Serial.println();
// // // // // }




// // // // // // void calibrateAccel()
// // // // // // {
// // // // // //     long sumX = 0;
// // // // // //     long sumY = 0;
// // // // // //     long sumZ = 0;

// // // // // //     Serial.println();
// // // // // //     Serial.println("KEEP MPU6050 FLAT");
// // // // // //     delay(2000);

// // // // // //     for(int i = 0; i < 2000; i++)
// // // // // //     {
// // // // // //         readMPU();

// // // // // //         sumX += AccX;
// // // // // //         sumY += AccY;
// // // // // //         sumZ += AccZ;

// // // // // //         delay(2);
// // // // // //     }

// // // // // //     float avgX = sumX / 2000.0f;
// // // // // //     float avgY = sumY / 2000.0f;
// // // // // //     float avgZ = sumZ / 2000.0f;

// // // // // //     // Vì đang đặt nằm ngang:
// // // // // //     // X = 0g
// // // // // //     // Y = 0g
// // // // // //     // Z = +1g = 16384

// // // // // //     AccOffsetX = avgX;
// // // // // //     AccOffsetY = avgY;
// // // // // //     AccOffsetZ = avgZ - 16384.0f;

// // // // // //     // Serial.println();
// // // // // //     // Serial.println("ACC CAL DONE");

// // // // // //     // Serial.print("Offset X = ");
// // // // // //     // Serial.println(AccOffsetX);

// // // // // //     // Serial.print("Offset Y = ");
// // // // // //     // Serial.println(AccOffsetY);

// // // // // //     // Serial.print("Offset Z = ");
// // // // // //     // Serial.println(AccOffsetZ);
// // // // // // }






// // // // // void setup()
// // // // // {
// // // // //     Serial.begin(115200);

// // // // //     Wire.begin(SDA_PIN,SCL_PIN);
// // // // //     Wire.setClock(400000);

// // // // //     delay(500);

// // // // //     initMPU();

// // // // //     calibrateGyro();
// // // // //     // calibrateAccel();
// // // // // }

// // // // // void loop()
// // // // // {
// // // // //     readMPU();

// // // // //     GyroRateX=(GyroX-GyroOffsetX)/131.0f;
// // // // //     GyroRateY=(GyroY-GyroOffsetY)/131.0f;
// // // // //     GyroRateZ=(GyroZ-GyroOffsetZ)/131.0f;


// // // // //     AccRateX = (AccX - AccOffsetX) / 16384.0f;
// // // // //    AccRateY = (AccY - AccOffsetY) / 16384.0f;
// // // // //   AccRateZ = (AccZ - AccOffsetZ) / 16384.0f;


// // // // //     Serial.print("GyroRateX = ");
// // // // // Serial.print(GyroRateX,3);

// // // // // Serial.print(" GyroRateY = ");
// // // // // Serial.print(GyroRateY,3);

// // // // // Serial.print(" GyroRateZ = ");
// // // // // Serial.print(GyroRateZ,3);


// // // // //     Serial.print("AccRateX = ");
// // // // // Serial.print(AccRateX,3);

// // // // // Serial.print(" AccRateY = ");
// // // // // Serial.print(AccRateY,3);

// // // // // Serial.print(" AccRateZ = ");
// // // // // Serial.println(AccRateZ,3);

// // // // //     // Serial.print("Gyro X : ");
// // // // //     // Serial.print(GyroRateX,3);

// // // // //     // Serial.print(" deg/s   ");

// // // // //     // Serial.print("Gyro Y : ");
// // // // //     // Serial.print(GyroRateY,3);

// // // // //     // Serial.print(" deg/s   ");

// // // // //     // Serial.print("Gyro Z : ");
// // // // //     // Serial.print(GyroRateZ,3);

// // // // //     // Serial.println(" deg/s");

// // // // //     delay(20);
// // // // // }


// // // // // #include <Arduino.h>
// // // // // #include <Wire.h>
// // // // // #include <WiFi.h>
// // // // // #include <esp_now.h>

// // // // // // Chân PWM cho từng motor
// // // // // int M1 = 3;
// // // // // int M2 = 6;
// // // // // int M3 = 5;
// // // // // int M4 = 4;
// // // // // float RateRoll, RatePitch, RateYaw;
// // // // // float RateCalibrationRoll, RateCalibrationPitch, RateCalibrationYaw;
// // // // // int RateCalibrationNumber;
// // // // // float ReceiverValue[]={0, 0, 0, 0, 0, 0, 0, 0};
// // // // // int ChannelNumber=0; 
// // // // // float Voltage, Current, BatteryRemaining, BatteryAtStart;
// // // // // float CurrentConsumed=0;
// // // // // float BatteryDefault=1300;
// // // // // uint32_t LoopTimer;
// // // // // float DesiredRateRoll, DesiredRatePitch,DesiredRateYaw;
// // // // // float ErrorRateRoll, ErrorRatePitch, ErrorRateYaw;
// // // // // float InputRoll, InputThrottle, InputPitch, InputYaw;
// // // // // float PrevErrorRateRoll, PrevErrorRatePitch, PrevErrorRateYaw;
// // // // // float PrevItermRateRoll, PrevItermRatePitch, PrevItermRateYaw;
// // // // // float PIDReturn[]={0, 0, 0};
// // // // // float PRateRoll= 1.14; float PRatePitch=PRateRoll; float PRateYaw=0;
// // // // // float IRateRoll=0.05; float IRatePitch=IRateRoll; float IRateYaw=0.5;
// // // // // float DRateRoll=0.03; float DRatePitch=DRateRoll; float DRateYaw=0.0;
// // // // // float MotorInput1, MotorInput2, MotorInput3, MotorInput4;
// // // // // float AccX, AccY, AccZ;
// // // // // float AngleRoll, AnglePitch;
// // // // // float KalmanAngleRoll=0, KalmanUncertaintyAngleRoll=2*2;
// // // // // float KalmanAnglePitch=0, KalmanUncertaintyAnglePitch=2*2;
// // // // // float Kalman1DOutput[]={0,0};
// // // // // float DesiredAngleRoll, DesiredAnglePitch;
// // // // // float ErrorAngleRoll, ErrorAnglePitch;
// // // // // float PrevErrorAngleRoll, PrevErrorAnglePitch;
// // // // // float PrevItermAngleRoll, PrevItermAnglePitch;
// // // // // float PAngleRoll=1.14; float PAnglePitch=PAngleRoll;
// // // // // float IAngleRoll=0.05; float IAnglePitch=IAngleRoll;
// // // // // float DAngleRoll=0.03; float DAnglePitch=DAngleRoll;
// // // // // void kalman_1d(float KalmanState, float KalmanUncertainty, float KalmanInput, float KalmanMeasurement) {
// // // // //   KalmanState=KalmanState+0.004*KalmanInput;
// // // // //   KalmanUncertainty=KalmanUncertainty + 0.004 * 0.004 * 4 * 4;
// // // // //   float KalmanGain=KalmanUncertainty * 1/(1*KalmanUncertainty + 3 * 3);
// // // // //   KalmanState=KalmanState+KalmanGain * (KalmanMeasurement-KalmanState);
// // // // //   KalmanUncertainty=(1-KalmanGain) * KalmanUncertainty;
// // // // //   Kalman1DOutput[0]=KalmanState; 
// // // // //   Kalman1DOutput[1]=KalmanUncertainty;
// // // // // }

// // // // // // ================= DATA =================
// // // // // typedef struct {
// // // // //   uint16_t ch[8];
// // // // // } Data;

// // // // // Data rx;

// // // // // // ================= STATE =================
// // // // // unsigned long lastRX = 0;

// // // // // bool connected = false;
// // // // // bool armed = false;

// // // // // uint8_t lostCount = 0;

// // // // // // ================= ESP-NOW CALLBACK =================
// // // // // void onRecv(const uint8_t *mac, const uint8_t *data, int len)
// // // // // {
// // // // //     memcpy(&rx, data, sizeof(rx));

// // // // //     lastRX = millis();
// // // // //     connected = true;
// // // // //     lostCount = 0;
// // // // // }

// // // // // void gyro_signals(void) {
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1A);
// // // // //   Wire.write(0x05);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1C);
// // // // //   Wire.write(0x10);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x3B);
// // // // //   Wire.endTransmission(); 
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   int16_t AccXLSB = Wire.read() << 8 | Wire.read();
// // // // //   int16_t AccYLSB = Wire.read() << 8 | Wire.read();
// // // // //   int16_t AccZLSB = Wire.read() << 8 | Wire.read();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1B); 
// // // // //   Wire.write(0x8);
// // // // //   Wire.endTransmission();                                                   
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x43);
// // // // //   Wire.endTransmission();
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   int16_t GyroX=Wire.read()<<8 | Wire.read();
// // // // //   int16_t GyroY=Wire.read()<<8 | Wire.read();
// // // // //   int16_t GyroZ=Wire.read()<<8 | Wire.read();
// // // // //   RateRoll=(float)GyroX/65.5;
// // // // //   RatePitch=(float)GyroY/65.5;
// // // // //   RateYaw=(float)GyroZ/65.5;
// // // // //   AccX=(float)AccXLSB/4096;
// // // // //   AccY=(float)AccYLSB/4096;
// // // // //   AccZ=(float)AccZLSB/4096;
// // // // //   AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
// // // // //   AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);
// // // // // }
// // // // // void pid_equation(float Error, float P , float I, float D, float PrevError, float PrevIterm) {
// // // // //   float Pterm=P*Error;
// // // // //   float Iterm=PrevIterm+I*(Error+PrevError)*0.004/2;
// // // // //   if (Iterm > 400) Iterm=400;
// // // // //   else if (Iterm <-400) Iterm=-400;
// // // // //     // if(Iterm>100) Iterm=100;
// // // // //     // if(Iterm<-100) Iterm=-100;

// // // // //   float Dterm=D*(Error-PrevError)/0.004;
// // // // //   float PIDOutput= Pterm+Iterm+Dterm;
// // // // //   if (PIDOutput>400) PIDOutput=400;
// // // // //   else if (PIDOutput <-400) PIDOutput=-400;
// // // // //     // if(PIDOutput > 100) PIDOutput = 100;
// // // // //     // if(PIDOutput < -100) PIDOutput = -100;
// // // // //   PIDReturn[0]=PIDOutput;
// // // // //   PIDReturn[1]=Error;
// // // // //   PIDReturn[2]=Iterm;
// // // // // }



// // // // // void reset_pid(void) {
// // // // //   PrevErrorRateRoll=0; PrevErrorRatePitch=0; PrevErrorRateYaw=0;
// // // // //   PrevItermRateRoll=0; PrevItermRatePitch=0; PrevItermRateYaw=0;
// // // // //   PrevErrorAngleRoll=0; PrevErrorAnglePitch=0;    
// // // // //   PrevItermAngleRoll=0; PrevItermAnglePitch=0;
// // // // // }
// // // // // void setup() {

// // // // //     Serial.begin(115200);
// // // // //     // pinMode(M1, OUTPUT);
// // // // //     // pinMode(M2, OUTPUT);
// // // // //     // pinMode(M3, OUTPUT);
// // // // //     // pinMode(M4, OUTPUT);

// // // // //     WiFi.mode(WIFI_STA);

// // // // //     if (esp_now_init() != ESP_OK)
// // // // //     {
// // // // //         Serial.println("ESP-NOW FAIL");
// // // // //         return;
// // // // //     }

// // // // //     esp_now_register_recv_cb(onRecv);

// // // // //     Serial.println("RX READY");

// // // // //   Wire.setClock(400000);
// // // // //   Wire.begin();
// // // // //   delay(250);
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x6B);
// // // // //   Wire.write(0x00);
// // // // //   Wire.endTransmission();
// // // // //   for (RateCalibrationNumber=0; 
// // // // //         RateCalibrationNumber<2000;
// // // // //         RateCalibrationNumber ++) {
// // // // //           gyro_signals();
// // // // //     RateCalibrationRoll+=RateRoll;
// // // // //     RateCalibrationPitch+=RatePitch;
// // // // //     RateCalibrationYaw+=RateYaw;
// // // // //     delay(1);
// // // // //   }
// // // // //   RateCalibrationRoll/=2000;
// // // // //   RateCalibrationPitch/=2000;
// // // // //   RateCalibrationYaw/=2000;
// // // // //   LoopTimer=micros();
// // // // // }

// // // // // void loop() {


// // // // //     gyro_signals();
// // // // //             RateRoll-=RateCalibrationRoll;
// // // // //             RatePitch-=RateCalibrationPitch;
// // // // //             RateYaw-=RateCalibrationYaw;
// // // // //             kalman_1d(KalmanAngleRoll, KalmanUncertaintyAngleRoll, RateRoll, AngleRoll);
// // // // //             KalmanAngleRoll=Kalman1DOutput[0]; KalmanUncertaintyAngleRoll=Kalman1DOutput[1];
// // // // //             kalman_1d(KalmanAnglePitch, KalmanUncertaintyAnglePitch, RatePitch, AnglePitch);
// // // // //             KalmanAnglePitch=Kalman1DOutput[0]; KalmanUncertaintyAnglePitch=Kalman1DOutput[1];
          

            
// // // // //             DesiredAngleRoll  = 0.10 * (rx.ch[3] - 1500);
// // // // //           DesiredAnglePitch = 0.10 * (rx.ch[2] - 1500);

       

// // // // //           // InputThrottle = map(rx.ch[0], 1000, 2000, 0, 255);
// // // // //           InputThrottle =(rx.ch[0]-1000)*255.0f/1000.0f;
// // // // //           InputThrottle = constrain(InputThrottle, 0, 255);

// // // // //           DesiredRateYaw = 0.15 * (rx.ch[1] - 1500);

// // // // //             ErrorAngleRoll=DesiredAngleRoll-KalmanAngleRoll;
// // // // //             ErrorAnglePitch=DesiredAnglePitch-KalmanAnglePitch;
// // // // //             pid_equation(ErrorAngleRoll, PAngleRoll, IAngleRoll, DAngleRoll, PrevErrorAngleRoll, PrevItermAngleRoll);     
// // // // //             DesiredRateRoll=PIDReturn[0]; 
// // // // //             PrevErrorAngleRoll=PIDReturn[1];
// // // // //             PrevItermAngleRoll=PIDReturn[2];
// // // // //             pid_equation(ErrorAnglePitch, PAnglePitch, IAnglePitch, DAnglePitch, PrevErrorAnglePitch, PrevItermAnglePitch);
// // // // //             DesiredRatePitch=PIDReturn[0]; 
// // // // //             PrevErrorAnglePitch=PIDReturn[1];
// // // // //             PrevItermAnglePitch=PIDReturn[2];
// // // // //             ErrorRateRoll=DesiredRateRoll-RateRoll;
// // // // //             ErrorRatePitch=DesiredRatePitch-RatePitch;
// // // // //             ErrorRateYaw=DesiredRateYaw-RateYaw;
// // // // //             pid_equation(ErrorRateRoll, PRateRoll, IRateRoll, DRateRoll, PrevErrorRateRoll, PrevItermRateRoll);
// // // // //                  InputRoll=PIDReturn[0];
// // // // //                  PrevErrorRateRoll=PIDReturn[1]; 
// // // // //                  PrevItermRateRoll=PIDReturn[2];
// // // // //             pid_equation(ErrorRatePitch, PRatePitch,IRatePitch, DRatePitch, PrevErrorRatePitch, PrevItermRatePitch);
// // // // //                  InputPitch=PIDReturn[0]; 
// // // // //                  PrevErrorRatePitch=PIDReturn[1]; 
// // // // //                  PrevItermRatePitch=PIDReturn[2];
// // // // //             pid_equation(ErrorRateYaw, PRateYaw,IRateYaw, DRateYaw, PrevErrorRateYaw, PrevItermRateYaw);
// // // // //                  InputYaw=PIDReturn[0]; 
// // // // //                  PrevErrorRateYaw=PIDReturn[1]; 
// // // // //                  PrevItermRateYaw=PIDReturn[2];

// // // // //             InputRoll  = constrain(InputRoll,  -60, 60);
// // // // //             InputPitch = constrain(InputPitch, -60, 60);
// // // // //             InputYaw   = constrain(InputYaw,   -40, 40);

           
// // // // //           MotorInput1 = 1.024*(InputThrottle + InputRoll - InputPitch - InputYaw);
// // // // //           MotorInput2 = 1.024*(InputThrottle - InputRoll - InputPitch + InputYaw);
// // // // //           MotorInput3 = 1.024*(InputThrottle - InputRoll + InputPitch - InputYaw);
// // // // //           MotorInput4 = 1.024*(InputThrottle + InputRoll + InputPitch + InputYaw);


// // // // //           MotorInput1 = constrain(MotorInput1,0,255);
// // // // //           MotorInput2 = constrain(MotorInput2,0,255);
// // // // //           MotorInput3 = constrain(MotorInput3,0,255);
// // // // //           MotorInput4 = constrain(MotorInput4,0,255);

// // // // //           // if(rx.ch[0] < 1050)
// // // // //           // {
// // // // //           //     MotorInput1 = 0;
// // // // //           //     MotorInput2 = 0;
// // // // //           //     MotorInput3 = 0;
// // // // //           //     MotorInput4 = 0;
// // // // //           //     reset_pid();
// // // // //           // }

// // // // //             // analogWrite(M1, MotorInput1);
// // // // //             // analogWrite(M2, MotorInput2);
// // // // //             // analogWrite(M3, MotorInput3);
// // // // //             // analogWrite(M4, MotorInput4);

// // // // //             //   Serial.print(" M1:"); Serial.print(rx.ch[0]);
// // // // //             // Serial.print(" M2:"); Serial.print(rx.ch[1]);
// // // // //             // Serial.print(" M3:"); Serial.print(rx.ch[2]);
// // // // //             // Serial.print(" M4:"); Serial.print(rx.ch[3]);
// // // // //             // Serial.print(" M5:"); Serial.println(rx.ch[4]);
            


          

// // // // //             Serial.print(" M1:"); Serial.print(MotorInput1);
// // // // //             Serial.print(" M2:"); Serial.print(MotorInput2);
// // // // //             Serial.print(" M3:"); Serial.print(MotorInput3);
// // // // //             Serial.print(" M4:"); Serial.println(MotorInput4);

// // // // //         while (micros() - LoopTimer < 4000);
// // // // //         LoopTimer=micros();

// // // // // }




// // // // // #include <Arduino.h>
// // // // // #include <Wire.h>
// // // // // #include <WiFi.h>

// // // // // int M1 = 3;
// // // // // int M2 = 6;
// // // // // int M3 = 5;
// // // // // int M4 = 4;
// // // // // float RateRoll, RatePitch, RateYaw;
// // // // // float RateCalibrationRoll, RateCalibrationPitch, RateCalibrationYaw;
// // // // // int RateCalibrationNumber;
// // // // // float ReceiverValue[]={0, 0, 0, 0, 0, 0, 0, 0};
// // // // // int ChannelNumber=0; 
// // // // // float Voltage, Current, BatteryRemaining, BatteryAtStart;
// // // // // float CurrentConsumed=0;
// // // // // float BatteryDefault=1300;
// // // // // uint32_t LoopTimer;
// // // // // float DesiredRateRoll, DesiredRatePitch,DesiredRateYaw;
// // // // // float ErrorRateRoll, ErrorRatePitch, ErrorRateYaw;
// // // // // float InputRoll, InputThrottle, InputPitch, InputYaw;
// // // // // float PrevErrorRateRoll, PrevErrorRatePitch, PrevErrorRateYaw;
// // // // // float PrevItermRateRoll, PrevItermRatePitch, PrevItermRateYaw;
// // // // // float PIDReturn[]={0, 0, 0};
// // // // // float PRateRoll= 1.14; float PRatePitch=PRateRoll; float PRateYaw=0;
// // // // // float IRateRoll=0.05; float IRatePitch=IRateRoll; float IRateYaw=0.5;
// // // // // float DRateRoll=0.03; float DRatePitch=DRateRoll; float DRateYaw=0.0;
// // // // // float MotorInput1, MotorInput2, MotorInput3, MotorInput4;
// // // // // float AccX, AccY, AccZ;
// // // // // float AngleRoll, AnglePitch;
// // // // // float KalmanAngleRoll=0, KalmanUncertaintyAngleRoll=2*2;
// // // // // float KalmanAnglePitch=0, KalmanUncertaintyAnglePitch=2*2;
// // // // // float Kalman1DOutput[]={0,0};
// // // // // float DesiredAngleRoll, DesiredAnglePitch;
// // // // // float ErrorAngleRoll, ErrorAnglePitch;
// // // // // float PrevErrorAngleRoll, PrevErrorAnglePitch;
// // // // // float PrevItermAngleRoll, PrevItermAnglePitch;
// // // // // float PAngleRoll=1.14; float PAnglePitch=PAngleRoll;
// // // // // float IAngleRoll=0.05; float IAnglePitch=IAngleRoll;
// // // // // float DAngleRoll=0.03; float DAnglePitch=DAngleRoll;
// // // // // int16_t AccXLSB;
// // // // // int16_t AccYLSB;
// // // // // int16_t AccZLSB;


// // // // // int16_t GyroX;
// // // // // int16_t GyroY;
// // // // // int16_t GyroZ;


// // // // // void gyro_signals(void) {
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1A);
// // // // //   Wire.write(0x05);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1C);
// // // // //   Wire.write(0x10);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x3B);
// // // // //   Wire.endTransmission(); 
// // // // //   Wire.requestFrom(0x68,6);

// // // // //   // int16_t AccXLSB = Wire.read() << 8 | Wire.read();
// // // // //   // int16_t AccYLSB = Wire.read() << 8 | Wire.read();
// // // // //   // int16_t AccZLSB = Wire.read() << 8 | Wire.read();


// // // // //     AccXLSB = Wire.read() << 8 | Wire.read();
// // // // //    AccYLSB = Wire.read() << 8 | Wire.read();
// // // // //    AccZLSB = Wire.read() << 8 | Wire.read();

  
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1B); 
// // // // //   Wire.write(0x8);
// // // // //   Wire.endTransmission();                                                   
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x43);
// // // // //   Wire.endTransmission();
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   // int16_t GyroX=Wire.read()<<8 | Wire.read();
// // // // //   // int16_t GyroY=Wire.read()<<8 | Wire.read();
// // // // //   // int16_t GyroZ=Wire.read()<<8 | Wire.read();


// // // // //   GyroX=Wire.read()<<8 | Wire.read();
// // // // //   GyroY=Wire.read()<<8 | Wire.read();
// // // // //   GyroZ=Wire.read()<<8 | Wire.read();
// // // // //   RateRoll=(float)GyroX/65.5;
// // // // //   RatePitch=(float)GyroY/65.5;
// // // // //   RateYaw=(float)GyroZ/65.5;
// // // // //   AccX=(float)AccXLSB/4096;
// // // // //   AccY=(float)AccYLSB/4096;
// // // // //   AccZ=(float)AccZLSB/4096;
// // // // //   AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
// // // // //   AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);
// // // // // }



// // // // // void setup() {

// // // // //     Serial.begin(115200);


// // // // //     Serial.println("RX READY");

// // // // //   Wire.setClock(400000);
// // // // //   Wire.begin();
// // // // //   delay(250);
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x6B);
// // // // //   Wire.write(0x00);
// // // // //   Wire.endTransmission();
// // // // //   for (RateCalibrationNumber=0; 
// // // // //         RateCalibrationNumber<500;
// // // // //         RateCalibrationNumber ++) {
// // // // //           gyro_signals();
// // // // //     RateCalibrationRoll+=RateRoll;
// // // // //     RateCalibrationPitch+=RatePitch;
// // // // //     RateCalibrationYaw+=RateYaw;
// // // // //     delay(1);
// // // // //   }
// // // // //   RateCalibrationRoll/=500;
// // // // //   RateCalibrationPitch/=500;
// // // // //   RateCalibrationYaw/=500;
 
// // // // // }

// // // // // void loop() {
// // // // //     gyro_signals();
// // // // //   RateRoll-=RateCalibrationRoll;
// // // // //   RatePitch-=RateCalibrationPitch;
// // // // //   RateYaw-=RateCalibrationYaw;

// // // // //             Serial.print(" AccXLSB:"); Serial.print(AccXLSB);
// // // // //             Serial.print(" AccYLSB:"); Serial.print(AccYLSB);
// // // // //             Serial.print(" AccZLSB:"); Serial.print(AccZLSB);
  


// // // // //                Serial.print(" RateRoll:"); Serial.print(RateRoll);
// // // // //             Serial.print(" RatePitch:"); Serial.print(RatePitch);
// // // // //             Serial.print(" RateYaw:"); Serial.print(RateYaw);



// // // // //                Serial.print(" AccX:"); Serial.print(AccX);
// // // // //             Serial.print(" AccY:"); Serial.print(AccY);
// // // // //             Serial.print(" AccZ:"); Serial.print(AccZ);


// // // // //                 Serial.print(" AngleRoll:"); Serial.print(AngleRoll);
// // // // //             Serial.print(" AnglePitch:"); Serial.println(AnglePitch);
            
          



// // // // // }






// // // // // #include <Arduino.h>
// // // // // #include <Wire.h>
// // // // // #include <WiFi.h>
// // // // // #include <esp_now.h>

// // // // // // Chân PWM cho từng motor
// // // // // int M1 = 3;
// // // // // int M2 = 6;
// // // // // int M3 = 5;
// // // // // int M4 = 4;
// // // // // float RateRoll, RatePitch, RateYaw;
// // // // // float RateCalibrationRoll, RateCalibrationPitch, RateCalibrationYaw;
// // // // // int RateCalibrationNumber;
// // // // // float ReceiverValue[]={0, 0, 0, 0, 0, 0, 0, 0};
// // // // // int ChannelNumber=0; 
// // // // // float Voltage, Current, BatteryRemaining, BatteryAtStart;
// // // // // float CurrentConsumed=0;
// // // // // float BatteryDefault=1300;
// // // // // uint32_t LoopTimer;
// // // // // float DesiredRateRoll, DesiredRatePitch,DesiredRateYaw;
// // // // // float ErrorRateRoll, ErrorRatePitch, ErrorRateYaw;
// // // // // float InputRoll, InputThrottle, InputPitch, InputYaw;
// // // // // float PrevErrorRateRoll, PrevErrorRatePitch, PrevErrorRateYaw;
// // // // // float PrevItermRateRoll, PrevItermRatePitch, PrevItermRateYaw;
// // // // // float PIDReturn[]={0, 0, 0};
// // // // // float PRateRoll= 1.14; float PRatePitch=PRateRoll; float PRateYaw=0;
// // // // // float IRateRoll=0.05; float IRatePitch=IRateRoll; float IRateYaw=0.5;
// // // // // float DRateRoll=0.03; float DRatePitch=DRateRoll; float DRateYaw=0.0;
// // // // // float MotorInput1, MotorInput2, MotorInput3, MotorInput4;
// // // // // float AccX, AccY, AccZ;
// // // // // float AngleRoll, AnglePitch;
// // // // // float KalmanAngleRoll=0, KalmanUncertaintyAngleRoll=2*2;
// // // // // float KalmanAnglePitch=0, KalmanUncertaintyAnglePitch=2*2;
// // // // // float Kalman1DOutput[]={0,0};
// // // // // float DesiredAngleRoll, DesiredAnglePitch;
// // // // // float ErrorAngleRoll, ErrorAnglePitch;
// // // // // float PrevErrorAngleRoll, PrevErrorAnglePitch;
// // // // // float PrevItermAngleRoll, PrevItermAnglePitch;
// // // // // float PAngleRoll=1.14; float PAnglePitch=PAngleRoll;
// // // // // float IAngleRoll=0.05; float IAnglePitch=IAngleRoll;
// // // // // float DAngleRoll=0.03; float DAnglePitch=DAngleRoll;
// // // // // void kalman_1d(float KalmanState, float KalmanUncertainty, float KalmanInput, float KalmanMeasurement) {
// // // // //   KalmanState=KalmanState+0.004*KalmanInput;
// // // // //   KalmanUncertainty=KalmanUncertainty + 0.004 * 0.004 * 4 * 4;
// // // // //   float KalmanGain=KalmanUncertainty * 1/(1*KalmanUncertainty + 3 * 3);
// // // // //   KalmanState=KalmanState+KalmanGain * (KalmanMeasurement-KalmanState);
// // // // //   KalmanUncertainty=(1-KalmanGain) * KalmanUncertainty;
// // // // //   Kalman1DOutput[0]=KalmanState; 
// // // // //   Kalman1DOutput[1]=KalmanUncertainty;
// // // // // }

// // // // // // ================= DATA =================
// // // // // typedef struct {
// // // // //   uint16_t ch[8];
// // // // // } Data;

// // // // // Data rx;

// // // // // // ================= STATE =================
// // // // // unsigned long lastRX = 0;

// // // // // bool connected = false;
// // // // // bool armed = false;

// // // // // uint8_t lostCount = 0;

// // // // // // ================= ESP-NOW CALLBACK =================
// // // // // void onRecv(const uint8_t *mac, const uint8_t *data, int len)
// // // // // {
// // // // //     memcpy(&rx, data, sizeof(rx));

// // // // //     lastRX = millis();
// // // // //     connected = true;
// // // // //     lostCount = 0;
// // // // // }

// // // // // void gyro_signals(void) {
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1A);
// // // // //   Wire.write(0x05);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1C);
// // // // //   Wire.write(0x10);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x3B);
// // // // //   Wire.endTransmission(); 
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   int16_t AccXLSB = Wire.read() << 8 | Wire.read();
// // // // //   int16_t AccYLSB = Wire.read() << 8 | Wire.read();
// // // // //   int16_t AccZLSB = Wire.read() << 8 | Wire.read();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1B); 
// // // // //   Wire.write(0x8);
// // // // //   Wire.endTransmission();                                                   
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x43);
// // // // //   Wire.endTransmission();
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   int16_t GyroX=Wire.read()<<8 | Wire.read();
// // // // //   int16_t GyroY=Wire.read()<<8 | Wire.read();
// // // // //   int16_t GyroZ=Wire.read()<<8 | Wire.read();
// // // // //   RateRoll=(float)GyroX/65.5;
// // // // //   RatePitch=(float)GyroY/65.5;
// // // // //   RateYaw=(float)GyroZ/65.5;
// // // // //   AccX=(float)AccXLSB/4096;
// // // // //   AccY=(float)AccYLSB/4096;
// // // // //   AccZ=(float)AccZLSB/4096;
// // // // //   AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
// // // // //   AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);
// // // // // }
// // // // // void pid_equation(float Error, float P , float I, float D, float PrevError, float PrevIterm) {
// // // // //   float Pterm=P*Error;
// // // // //   float Iterm=PrevIterm+I*(Error+PrevError)*0.004/2;
// // // // //   if (Iterm > 400) Iterm=400;
// // // // //   else if (Iterm <-400) Iterm=-400;
// // // // //     // if(Iterm>100) Iterm=100;
// // // // //     // if(Iterm<-100) Iterm=-100;

// // // // //   float Dterm=D*(Error-PrevError)/0.004;
// // // // //   float PIDOutput= Pterm+Iterm+Dterm;
// // // // //   if (PIDOutput>400) PIDOutput=400;
// // // // //   else if (PIDOutput <-400) PIDOutput=-400;
// // // // //     // if(PIDOutput > 100) PIDOutput = 100;
// // // // //     // if(PIDOutput < -100) PIDOutput = -100;
// // // // //   PIDReturn[0]=PIDOutput;
// // // // //   PIDReturn[1]=Error;
// // // // //   PIDReturn[2]=Iterm;
// // // // // }



// // // // // void reset_pid(void) {
// // // // //   PrevErrorRateRoll=0; PrevErrorRatePitch=0; PrevErrorRateYaw=0;
// // // // //   PrevItermRateRoll=0; PrevItermRatePitch=0; PrevItermRateYaw=0;
// // // // //   PrevErrorAngleRoll=0; PrevErrorAnglePitch=0;    
// // // // //   PrevItermAngleRoll=0; PrevItermAnglePitch=0;
// // // // // }
// // // // // void setup() {

// // // // //     Serial.begin(115200);
// // // // //     pinMode(M1, OUTPUT);
// // // // //     pinMode(M2, OUTPUT);
// // // // //     pinMode(M3, OUTPUT);
// // // // //     pinMode(M4, OUTPUT);

// // // // //     WiFi.mode(WIFI_STA);

// // // // //     if (esp_now_init() != ESP_OK)
// // // // //     {
// // // // //         Serial.println("ESP-NOW FAIL");
// // // // //         return;
// // // // //     }

// // // // //     esp_now_register_recv_cb(onRecv);

// // // // //     Serial.println("RX READY");

// // // // //   Wire.setClock(400000);
// // // // //   Wire.begin();
// // // // //   delay(250);
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x6B);
// // // // //   Wire.write(0x00);
// // // // //   Wire.endTransmission();
// // // // //   for (RateCalibrationNumber=0; 
// // // // //         RateCalibrationNumber<2000;
// // // // //         RateCalibrationNumber ++) {
// // // // //           gyro_signals();
// // // // //     RateCalibrationRoll+=RateRoll;
// // // // //     RateCalibrationPitch+=RatePitch;
// // // // //     RateCalibrationYaw+=RateYaw;
// // // // //     delay(1);
// // // // //   }
// // // // //   RateCalibrationRoll/=2000;
// // // // //   RateCalibrationPitch/=2000;
// // // // //   RateCalibrationYaw/=2000;
// // // // //   LoopTimer=micros();
// // // // // }

// // // // // void loop() {


// // // // //    // ================= CHECK LOST TX =================
// // // // //     if (connected && millis() - lastRX > 300)
// // // // //     {
// // // // //         lostCount++;

// // // // //         if (lostCount >= 5)
// // // // //         {
// // // // //             connected = false;

// // // // //             if (armed)
// // // // //             {
// // // // //                 armed = false;
// // // // //                  reset_pid();
// // // // //                  KalmanAngleRoll = 0;
// // // // //                   KalmanAnglePitch = 0;

// // // // //                   KalmanUncertaintyAngleRoll = 4;
// // // // //                   KalmanUncertaintyAnglePitch = 4;
// // // // //                 Serial.println("AUTO DISARM");
// // // // //             }

// // // // //             Serial.println("TX DISCONNECTED");
// // // // //         }

// // // // //         lastRX = millis();
// // // // //     }


// // // // //        //         // ARM  khi giam xuong
// // // // //       if (connected && !armed && rx.ch[4] < 1050)
// // // // //       {
// // // // //           armed = true;
// // // // //           Serial.println("ARM");

// // // // //       }

// // // // //       // DISARM
// // // // //       if (armed && rx.ch[4] > 1800)
// // // // //       {
// // // // //           armed = false;
          
// // // // //           Serial.println("DISARM");
// // // // //       }


// // // // //       if(armed){

// // // //           //   gyro_signals();
// // // //           //   RateRoll-=RateCalibrationRoll;
// // // //           //   RatePitch-=RateCalibrationPitch;
// // // //           //   RateYaw-=RateCalibrationYaw;
// // // //           //   kalman_1d(KalmanAngleRoll, KalmanUncertaintyAngleRoll, RateRoll, AngleRoll);
// // // //           //   KalmanAngleRoll=Kalman1DOutput[0]; KalmanUncertaintyAngleRoll=Kalman1DOutput[1];
// // // //           //   kalman_1d(KalmanAnglePitch, KalmanUncertaintyAnglePitch, RatePitch, AnglePitch);
// // // //           //   KalmanAnglePitch=Kalman1DOutput[0]; KalmanUncertaintyAnglePitch=Kalman1DOutput[1];
          

            
// // // //           //   DesiredAngleRoll  = 0.10 * (rx.ch[3] - 1500);
// // // //           // DesiredAnglePitch = 0.10 * (rx.ch[2] - 1500);

       

// // // //           // // InputThrottle = map(rx.ch[0], 1000, 2000, 0, 255);
// // // //           // InputThrottle =(rx.ch[0]-1000)*255.0f/1000.0f;
// // // //           // InputThrottle = constrain(InputThrottle, 0, 255);

// // // //           // DesiredRateYaw = 0.15 * (rx.ch[1] - 1500);

// // // //           //   ErrorAngleRoll=DesiredAngleRoll-KalmanAngleRoll;
// // // //           //   ErrorAnglePitch=DesiredAnglePitch-KalmanAnglePitch;
// // // //           //   pid_equation(ErrorAngleRoll, PAngleRoll, IAngleRoll, DAngleRoll, PrevErrorAngleRoll, PrevItermAngleRoll);     
// // // //           //   DesiredRateRoll=PIDReturn[0]; 
// // // //           //   PrevErrorAngleRoll=PIDReturn[1];
// // // //           //   PrevItermAngleRoll=PIDReturn[2];
// // // //           //   pid_equation(ErrorAnglePitch, PAnglePitch, IAnglePitch, DAnglePitch, PrevErrorAnglePitch, PrevItermAnglePitch);
// // // //           //   DesiredRatePitch=PIDReturn[0]; 
// // // //           //   PrevErrorAnglePitch=PIDReturn[1];
// // // //           //   PrevItermAnglePitch=PIDReturn[2];
// // // //           //   ErrorRateRoll=DesiredRateRoll-RateRoll;
// // // //           //   ErrorRatePitch=DesiredRatePitch-RatePitch;
// // // //           //   ErrorRateYaw=DesiredRateYaw-RateYaw;
// // // //           //   pid_equation(ErrorRateRoll, PRateRoll, IRateRoll, DRateRoll, PrevErrorRateRoll, PrevItermRateRoll);
// // // //           //        InputRoll=PIDReturn[0];
// // // //           //        PrevErrorRateRoll=PIDReturn[1]; 
// // // //           //        PrevItermRateRoll=PIDReturn[2];
// // // //           //   pid_equation(ErrorRatePitch, PRatePitch,IRatePitch, DRatePitch, PrevErrorRatePitch, PrevItermRatePitch);
// // // //           //        InputPitch=PIDReturn[0]; 
// // // //           //        PrevErrorRatePitch=PIDReturn[1]; 
// // // //           //        PrevItermRatePitch=PIDReturn[2];
// // // //           //   pid_equation(ErrorRateYaw, PRateYaw,IRateYaw, DRateYaw, PrevErrorRateYaw, PrevItermRateYaw);
// // // //           //        InputYaw=PIDReturn[0]; 
// // // //           //        PrevErrorRateYaw=PIDReturn[1]; 
// // // //           //        PrevItermRateYaw=PIDReturn[2];

// // // //           //   InputRoll  = constrain(InputRoll,  -60, 60);
// // // //           //   InputPitch = constrain(InputPitch, -60, 60);
// // // //           //   InputYaw   = constrain(InputYaw,   -40, 40);

           
// // // //           // MotorInput1 = 1.024*(InputThrottle + InputRoll - InputPitch - InputYaw);
// // // //           // MotorInput2 = 1.024*(InputThrottle - InputRoll - InputPitch + InputYaw);
// // // //           // MotorInput3 = 1.024*(InputThrottle - InputRoll + InputPitch - InputYaw);
// // // //           // MotorInput4 = 1.024*(InputThrottle + InputRoll + InputPitch + InputYaw);


// // // //           // MotorInput1 = constrain(MotorInput1,0,255);
// // // //           // MotorInput2 = constrain(MotorInput2,0,255);
// // // //           // MotorInput3 = constrain(MotorInput3,0,255);
// // // //           // MotorInput4 = constrain(MotorInput4,0,255);

// // // // //           if(rx.ch[0] < 1050)
// // // // //           {
// // // // //               MotorInput1 = 0;
// // // // //               MotorInput2 = 0;
// // // // //               MotorInput3 = 0;
// // // // //               MotorInput4 = 0;
// // // // //               reset_pid();
// // // // //           }

// // // // //             analogWrite(M1, MotorInput1);
// // // // //             analogWrite(M2, MotorInput2);
// // // // //             analogWrite(M3, MotorInput3);
// // // // //             analogWrite(M4, MotorInput4);

// // // // //             //   Serial.print(" M1:"); Serial.print(rx.ch[0]);
// // // // //             // Serial.print(" M2:"); Serial.print(rx.ch[1]);
// // // // //             // Serial.print(" M3:"); Serial.print(rx.ch[2]);
// // // // //             // Serial.print(" M4:"); Serial.print(rx.ch[3]);
// // // // //             // Serial.print(" M5:"); Serial.println(rx.ch[4]);
            


          

// // // // //             Serial.print(" M1:"); Serial.print(MotorInput1);
// // // // //             Serial.print(" M2:"); Serial.print(MotorInput2);
// // // // //             Serial.print(" M3:"); Serial.print(MotorInput3);
// // // // //             Serial.print(" M4:"); Serial.println(MotorInput4);


          
// // // // //       }

// // // // //       if(!armed)
// // // // //       {
// // // // //           reset_pid();

// // // // //           MotorInput1=0;
// // // // //           MotorInput2=0;
// // // // //           MotorInput3=0;
// // // // //           MotorInput4=0;

// // // // //           analogWrite(M1,0);
// // // // //           analogWrite(M2,0);
// // // // //           analogWrite(M3,0);
// // // // //           analogWrite(M4,0);
// // // // //       }

// // // // //         while (micros() - LoopTimer < 4000);
// // // // //         LoopTimer=micros();

// // // // // }





// // // // // #include <Arduino.h>
// // // // // #include <Wire.h>
// // // // // #include <WiFi.h>
// // // // // #include <esp_now.h>

// // // // // // Chân PWM cho từng motor
// // // // // int M1 = 4;
// // // // // int M2 = 5;
// // // // // int M3 = 3;
// // // // // int M4 = 6;
// // // // // float RateRoll, RatePitch, RateYaw;
// // // // // float RateCalibrationRoll, RateCalibrationPitch, RateCalibrationYaw;
// // // // // int RateCalibrationNumber;
// // // // // float ReceiverValue[]={0, 0, 0, 0, 0, 0, 0, 0};
// // // // // int ChannelNumber=0; 
// // // // // float Voltage, Current, BatteryRemaining, BatteryAtStart;
// // // // // float CurrentConsumed=0;
// // // // // float BatteryDefault=1300;
// // // // // uint32_t LoopTimer;
// // // // // float DesiredRateRoll, DesiredRatePitch,DesiredRateYaw;
// // // // // float ErrorRateRoll, ErrorRatePitch, ErrorRateYaw;
// // // // // float InputRoll, InputThrottle, InputPitch, InputYaw;
// // // // // float PrevErrorRateRoll, PrevErrorRatePitch, PrevErrorRateYaw;
// // // // // float PrevItermRateRoll, PrevItermRatePitch, PrevItermRateYaw;
// // // // // float PIDReturn[]={0, 0, 0};
// // // // // float PRateRoll=0.3; float PRatePitch=PRateRoll; float PRateYaw=0;
// // // // // float IRateRoll=0.0; float IRatePitch=IRateRoll; float IRateYaw=0;
// // // // // float DRateRoll=0.0; float DRatePitch=DRateRoll; float DRateYaw=0;
// // // // // float MotorInput1, MotorInput2, MotorInput3, MotorInput4;
// // // // // float AccX, AccY, AccZ;
// // // // // float AngleRoll, AnglePitch;
// // // // // float KalmanAngleRoll=0, KalmanUncertaintyAngleRoll=2*2;
// // // // // float KalmanAnglePitch=0, KalmanUncertaintyAnglePitch=2*2;
// // // // // float Kalman1DOutput[]={0,0};
// // // // // float DesiredAngleRoll, DesiredAnglePitch;
// // // // // float ErrorAngleRoll, ErrorAnglePitch;
// // // // // float PrevErrorAngleRoll, PrevErrorAnglePitch;
// // // // // float PrevItermAngleRoll, PrevItermAnglePitch;
// // // // // float PAngleRoll=1; float PAnglePitch=PAngleRoll;
// // // // // float IAngleRoll=0; float IAnglePitch=IAngleRoll;
// // // // // float DAngleRoll=0; float DAnglePitch=DAngleRoll;
// // // // // void kalman_1d(float KalmanState, float KalmanUncertainty, float KalmanInput, float KalmanMeasurement) {
// // // // //   KalmanState=KalmanState+0.004*KalmanInput;
// // // // //   KalmanUncertainty=KalmanUncertainty + 0.004 * 0.004 * 4 * 4;
// // // // //   float KalmanGain=KalmanUncertainty * 1/(1*KalmanUncertainty + 3 * 3);
// // // // //   KalmanState=KalmanState+KalmanGain * (KalmanMeasurement-KalmanState);
// // // // //   KalmanUncertainty=(1-KalmanGain) * KalmanUncertainty;
// // // // //   Kalman1DOutput[0]=KalmanState; 
// // // // //   Kalman1DOutput[1]=KalmanUncertainty;
// // // // // }
// // // // // // void battery_voltage(void) {
// // // // // //   Voltage=(float)analogRead(15)/62;
// // // // // //   Current=(float)analogRead(21)*0.089;
// // // // // // }
// // // // // // void read_receiver(void){
// // // // // //   ChannelNumber = ReceiverInput.available();  
// // // // // //   if (ChannelNumber > 0) {
// // // // // //          for (int i=1; i<=ChannelNumber;i++){
// // // // // //               ReceiverValue[i-1]=ReceiverInput.read(i);
// // // // // //          }
// // // // // //   }
// // // // // // }


// // // // // // ================= DATA =================
// // // // // typedef struct {
// // // // //   uint16_t ch[8];
// // // // // } Data;

// // // // // Data rx;

// // // // // // ================= STATE =================
// // // // // unsigned long lastRX = 0;

// // // // // bool connected = false;
// // // // // bool armed = false;

// // // // // uint8_t lostCount = 0;

// // // // // // ================= ESP-NOW CALLBACK =================
// // // // // void onRecv(const uint8_t *mac, const uint8_t *data, int len)
// // // // // {
// // // // //     memcpy(&rx, data, sizeof(rx));

// // // // //     lastRX = millis();
// // // // //     connected = true;
// // // // //     lostCount = 0;
// // // // // }

// // // // // void gyro_signals(void) {
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1A);
// // // // //   Wire.write(0x05);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1C);
// // // // //   Wire.write(0x10);
// // // // //   Wire.endTransmission();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x3B);
// // // // //   Wire.endTransmission(); 
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   int16_t AccXLSB = Wire.read() << 8 | Wire.read();
// // // // //   int16_t AccYLSB = Wire.read() << 8 | Wire.read();
// // // // //   int16_t AccZLSB = Wire.read() << 8 | Wire.read();
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x1B); 
// // // // //   Wire.write(0x8);
// // // // //   Wire.endTransmission();                                                   
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x43);
// // // // //   Wire.endTransmission();
// // // // //   Wire.requestFrom(0x68,6);
// // // // //   int16_t GyroX=Wire.read()<<8 | Wire.read();
// // // // //   int16_t GyroY=Wire.read()<<8 | Wire.read();
// // // // //   int16_t GyroZ=Wire.read()<<8 | Wire.read();
// // // // //   RateRoll=(float)GyroX/65.5;
// // // // //   RatePitch=(float)GyroY/65.5;
// // // // //   RateYaw=(float)GyroZ/65.5;
// // // // //   AccX=(float)AccXLSB/4096;
// // // // //   AccY=(float)AccYLSB/4096;
// // // // //   AccZ=(float)AccZLSB/4096;
// // // // //   AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
// // // // //   AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);
// // // // // }
// // // // // void pid_equation(float Error, float P , float I, float D, float PrevError, float PrevIterm) {
// // // // //   float Pterm=P*Error;
// // // // //   float Iterm=PrevIterm+I*(Error+PrevError)*0.004/2;
// // // // // //   if (Iterm > 400) Iterm=400;
// // // // // //   else if (Iterm <-400) Iterm=-400;
// // // // //     if(Iterm>100) Iterm=100;
// // // // //     if(Iterm<-100) Iterm=-100;

// // // // //   float Dterm=D*(Error-PrevError)/0.004;
// // // // //   float PIDOutput= Pterm+Iterm+Dterm;
// // // // // //   if (PIDOutput>400) PIDOutput=400;
// // // // // //   else if (PIDOutput <-400) PIDOutput=-400;
// // // // //     if(PIDOutput > 100) PIDOutput = 100;
// // // // //     if(PIDOutput < -100) PIDOutput = -100;
// // // // //   PIDReturn[0]=PIDOutput;
// // // // //   PIDReturn[1]=Error;
// // // // //   PIDReturn[2]=Iterm;
// // // // // }



// // // // // void reset_pid(void) {
// // // // //   PrevErrorRateRoll=0; PrevErrorRatePitch=0; PrevErrorRateYaw=0;
// // // // //   PrevItermRateRoll=0; PrevItermRatePitch=0; PrevItermRateYaw=0;
// // // // //   PrevErrorAngleRoll=0; PrevErrorAnglePitch=0;    
// // // // //   PrevItermAngleRoll=0; PrevItermAnglePitch=0;
// // // // // }
// // // // // void setup() {

// // // // //     Serial.begin(115200);
// // // // //     pinMode(M1, OUTPUT);
// // // // //     pinMode(M2, OUTPUT);
// // // // //     pinMode(M3, OUTPUT);
// // // // //     pinMode(M4, OUTPUT);

// // // // //     WiFi.mode(WIFI_STA);

// // // // //     if (esp_now_init() != ESP_OK)
// // // // //     {
// // // // //         Serial.println("ESP-NOW FAIL");
// // // // //         return;
// // // // //     }

// // // // //     esp_now_register_recv_cb(onRecv);

// // // // //     Serial.println("RX READY");

// // // // //   Wire.setClock(400000);
// // // // //   Wire.begin();
// // // // //   delay(250);
// // // // //   Wire.beginTransmission(0x68);
// // // // //   Wire.write(0x6B);
// // // // //   Wire.write(0x00);
// // // // //   Wire.endTransmission();
// // // // //   for (RateCalibrationNumber=0; 
// // // // //         RateCalibrationNumber<2000;
// // // // //         RateCalibrationNumber ++) {
// // // // //           gyro_signals();
// // // // //     RateCalibrationRoll+=RateRoll;
// // // // //     RateCalibrationPitch+=RatePitch;
// // // // //     RateCalibrationYaw+=RateYaw;
// // // // //     delay(1);
// // // // //   }
// // // // //   RateCalibrationRoll/=2000;
// // // // //   RateCalibrationPitch/=2000;
// // // // //   RateCalibrationYaw/=2000;
// // // // // //   analogWriteFrequency(1, 250);
// // // // // //   analogWriteFrequency(2, 250);
// // // // // //   analogWriteFrequency(3, 250);
// // // // // //   analogWriteFrequency(4, 250);
// // // // // //   analogWriteResolution(12);
// // // // // //   pinMode(6, OUTPUT);
// // // // // //   digitalWrite(6, HIGH);
// // // // // //   battery_voltage();
// // // // // //   if (Voltage > 8.3) { digitalWrite(5, LOW); BatteryAtStart=BatteryDefault; }
// // // // // //   else if (Voltage < 7.5) {BatteryAtStart=30/100*BatteryDefault ;}
// // // // // //   else { digitalWrite(5, LOW); BatteryAtStart=(82*Voltage-580)/100*BatteryDefault; }
// // // // // //   ReceiverInput.begin(14);
// // // // // //   while (ReceiverValue[2] < 1020 || 
// // // // // //          ReceiverValue[2] > 1050) {
// // // // // //     read_receiver();
// // // // // //     delay(4);
// // // // // //   }
// // // // //   LoopTimer=micros();
// // // // // }
// // // // // void loop() {
// // // // //   gyro_signals();
// // // // //   RateRoll-=RateCalibrationRoll;
// // // // //   RatePitch-=RateCalibrationPitch;
// // // // //   RateYaw-=RateCalibrationYaw;
// // // // //   kalman_1d(KalmanAngleRoll, KalmanUncertaintyAngleRoll, RateRoll, AngleRoll);
// // // // //   KalmanAngleRoll=Kalman1DOutput[0]; KalmanUncertaintyAngleRoll=Kalman1DOutput[1];
// // // // //   kalman_1d(KalmanAnglePitch, KalmanUncertaintyAnglePitch, RatePitch, AnglePitch);
// // // // //   KalmanAnglePitch=Kalman1DOutput[0]; KalmanUncertaintyAnglePitch=Kalman1DOutput[1];
// // // // // //   read_receiver();
// // // // // //   DesiredAngleRoll=0.10 * (rx.ch[3] - 1500);
// // // // // //   DesiredAnglePitch=0.10*(rx.ch[3]-1500);
// // // // // //   InputThrottle=rx.ch[3];
// // // // // //   DesiredRateYaw=0.15*(rx.ch[3]-1500);
  
// // // // //   DesiredAngleRoll  = 0.10 * (rx.ch[3] - 1500);
// // // // // DesiredAnglePitch = 0.10 * (rx.ch[2] - 1500);

// // // // // // Dung cho motor brushlless
// // // // // // InputThrottle = rx.ch[0];

// // // // // InputThrottle = map(rx.ch[0], 1000, 2000, 0, 255);
// // // // // InputThrottle = constrain(InputThrottle, 0, 255);

// // // // // DesiredRateYaw = 0.15 * (rx.ch[1] - 1500);

// // // // //   ErrorAngleRoll=DesiredAngleRoll-KalmanAngleRoll;
// // // // //   ErrorAnglePitch=DesiredAnglePitch-KalmanAnglePitch;
// // // // //   pid_equation(ErrorAngleRoll, PAngleRoll, IAngleRoll, DAngleRoll, PrevErrorAngleRoll, PrevItermAngleRoll);     
// // // // //   DesiredRateRoll=PIDReturn[0]; 
// // // // //   PrevErrorAngleRoll=PIDReturn[1];
// // // // //   PrevItermAngleRoll=PIDReturn[2];
// // // // //   pid_equation(ErrorAnglePitch, PAnglePitch, IAnglePitch, DAnglePitch, PrevErrorAnglePitch, PrevItermAnglePitch);
// // // // //   DesiredRatePitch=PIDReturn[0]; 
// // // // //   PrevErrorAnglePitch=PIDReturn[1];
// // // // //   PrevItermAnglePitch=PIDReturn[2];
// // // // //   ErrorRateRoll=DesiredRateRoll-RateRoll;
// // // // //   ErrorRatePitch=DesiredRatePitch-RatePitch;
// // // // //   ErrorRateYaw=DesiredRateYaw-RateYaw;
// // // // //   pid_equation(ErrorRateRoll, PRateRoll, IRateRoll, DRateRoll, PrevErrorRateRoll, PrevItermRateRoll);
// // // // //        InputRoll=PIDReturn[0];
// // // // //        PrevErrorRateRoll=PIDReturn[1]; 
// // // // //        PrevItermRateRoll=PIDReturn[2];
// // // // //   pid_equation(ErrorRatePitch, PRatePitch,IRatePitch, DRatePitch, PrevErrorRatePitch, PrevItermRatePitch);
// // // // //        InputPitch=PIDReturn[0]; 
// // // // //        PrevErrorRatePitch=PIDReturn[1]; 
// // // // //        PrevItermRatePitch=PIDReturn[2];
// // // // //   pid_equation(ErrorRateYaw, PRateYaw,IRateYaw, DRateYaw, PrevErrorRateYaw, PrevItermRateYaw);
// // // // //        InputYaw=PIDReturn[0]; 
// // // // //        PrevErrorRateYaw=PIDReturn[1]; 
// // // // //        PrevItermRateYaw=PIDReturn[2];

// // // // //   InputRoll  = constrain(InputRoll,  -60, 60);
// // // // //   InputPitch = constrain(InputPitch, -60, 60);
// // // // //   InputYaw   = constrain(InputYaw,   -40, 40);

// // // // //   // Dung cho motor brushlless
// // // // // //   if (InputThrottle > 1800) InputThrottle = 1800;
// // // // // //    MotorInput1= 1.024*(InputThrottle+InputRoll-InputPitch-InputYaw);
// // // // // //   MotorInput2= 1.024*(InputThrottle-InputRoll-InputPitch+InputYaw);
// // // // // //   MotorInput3= 1.024*(InputThrottle-InputRoll+InputPitch-InputYaw);
// // // // // //   MotorInput4= 1.024*(InputThrottle+InputRoll+InputPitch+InputYaw);


// // // // // //   MotorInput1= 1.024*(InputThrottle-InputRoll-InputPitch-InputYaw);
// // // // // //   MotorInput2= 1.024*(InputThrottle-InputRoll+InputPitch+InputYaw);
// // // // // //   MotorInput3= 1.024*(InputThrottle+InputRoll+InputPitch-InputYaw);
// // // // // //   MotorInput4= 1.024*(InputThrottle+InputRoll-InputPitch+InputYaw);


// // // // // //    MotorInput1= 1.024*(InputThrottle+InputRoll);
// // // // // //   MotorInput2= 1.024*(InputThrottle-InputRoll);
// // // // // //   MotorInput3= 1.024*(InputThrottle-InputRoll);
// // // // // //   MotorInput4= 1.024*(InputThrottle+InputRoll);
   

// // // // // //  MotorInput1= 1.024*(InputThrottle-InputPitch);
// // // // // //   MotorInput2= 1.024*(InputThrottle-InputPitch);
// // // // // //   MotorInput3= 1.024*(InputThrottle+InputPitch);
// // // // // //   MotorInput4= 1.024*(InputThrottle+InputPitch);


// // // // // //  MotorInput1= 1.024*(InputThrottle-InputYaw);
// // // // // //   MotorInput2= 1.024*(InputThrottle+InputYaw);
// // // // // //   MotorInput3= 1.024*(InputThrottle-InputYaw);
// // // // // //   MotorInput4= 1.024*(InputThrottle+InputYaw);

  



// // // // // MotorInput1 = InputThrottle + InputRoll - InputPitch - InputYaw;
// // // // // MotorInput2 = InputThrottle - InputRoll - InputPitch + InputYaw;
// // // // // MotorInput3 = InputThrottle - InputRoll + InputPitch - InputYaw;
// // // // // MotorInput4 = InputThrottle + InputRoll + InputPitch + InputYaw;


// // // // // MotorInput1 = constrain(MotorInput1,0,255);
// // // // // MotorInput2 = constrain(MotorInput2,0,255);
// // // // // MotorInput3 = constrain(MotorInput3,0,255);
// // // // // MotorInput4 = constrain(MotorInput4,0,255);

// // // // // if(rx.ch[0] < 1050)
// // // // // {
// // // // //     MotorInput1 = 0;
// // // // //     MotorInput2 = 0;
// // // // //     MotorInput3 = 0;
// // // // //     MotorInput4 = 0;
// // // // //     reset_pid();
// // // // // }

// // // // //   analogWrite(M1, MotorInput1);
// // // // //   analogWrite(M2, MotorInput2);
// // // // //   analogWrite(M3, MotorInput3);
// // // // //   analogWrite(M4, MotorInput4);



// // // // // //     // Dung cho motor brushlless
// // // // // //   if (MotorInput1 > 2000)MotorInput1 = 1999;
// // // // // //   if (MotorInput2 > 2000)MotorInput2 = 1999; 
// // // // // //   if (MotorInput3 > 2000)MotorInput3 = 1999; 
// // // // // //   if (MotorInput4 > 2000)MotorInput4 = 1999;
// // // // // //   int ThrottleIdle=1180;
// // // // // //   if (MotorInput1 < ThrottleIdle) MotorInput1 = ThrottleIdle;
// // // // // //   if (MotorInput2 < ThrottleIdle) MotorInput2 = ThrottleIdle;
// // // // // //   if (MotorInput3 < ThrottleIdle) MotorInput3 = ThrottleIdle;
// // // // // //   if (MotorInput4 < ThrottleIdle) MotorInput4 = ThrottleIdle;
// // // // // //   int ThrottleCutOff=1000;
// // // // // //   if (rx.ch[0] < 1050) {
// // // // // //     MotorInput1=ThrottleCutOff; 
// // // // // //     MotorInput2=ThrottleCutOff;
// // // // // //     MotorInput3=ThrottleCutOff; 
// // // // // //     MotorInput4=ThrottleCutOff;
// // // // // //     reset_pid();
// // // // // //   }

// // // // // //   Serial.print(rx.ch[0]);

// // // // // //   Serial.print(rx.ch[1]);
// // // // // //   Serial.print(rx.ch[2]);
// // // // // //   Serial.println(rx.ch[3]);


// // // // // //     Serial.print(" M1:"); Serial.print(rx.ch[0]);
// // // // // //   Serial.print(" M2:"); Serial.print(rx.ch[1]);
// // // // // //   Serial.print(" M3:"); Serial.print(rx.ch[2]);
// // // // // //   Serial.print(" M4:"); Serial.println(rx.ch[3]);

 

// // // // //   Serial.print(" M1:"); Serial.print(MotorInput1);
// // // // //   Serial.print(" M2:"); Serial.print(MotorInput2);
// // // // //   Serial.print(" M3:"); Serial.print(MotorInput3);
// // // // //   Serial.print(" M4:"); Serial.println(MotorInput4);




// // // // // //   analogWrite(1,MotorInput1);
// // // // // //   analogWrite(2,MotorInput2);
// // // // // //   analogWrite(3,MotorInput3); 
// // // // // //   analogWrite(4,MotorInput4);
// // // // // //   battery_voltage();
// // // // // //   CurrentConsumed=Current*1000*0.004/3600+CurrentConsumed;
// // // // // //   BatteryRemaining=(BatteryAtStart-CurrentConsumed)/BatteryDefault*100;
// // // // // //   if (BatteryRemaining<=30) digitalWrite(5, HIGH);
// // // // // //   else digitalWrite(5, LOW);
// // // // //   while (micros() - LoopTimer < 4000);
// // // // //   LoopTimer=micros();
// // // // // }











































// // // // // void loop() {
// // // // //   gyro_signals();
// // // // //   RateRoll-=RateCalibrationRoll;
// // // // //   RatePitch-=RateCalibrationPitch;
// // // // //   RateYaw-=RateCalibrationYaw;
// // // // //   kalman_1d(KalmanAngleRoll, KalmanUncertaintyAngleRoll, RateRoll, AngleRoll);
// // // // //   KalmanAngleRoll=Kalman1DOutput[0]; KalmanUncertaintyAngleRoll=Kalman1DOutput[1];
// // // // //   kalman_1d(KalmanAnglePitch, KalmanUncertaintyAnglePitch, RatePitch, AnglePitch);
// // // // //   KalmanAnglePitch=Kalman1DOutput[0]; KalmanUncertaintyAnglePitch=Kalman1DOutput[1];
// // // // // //   read_receiver();
// // // // // //   DesiredAngleRoll=0.10 * (rx.ch[3] - 1500);
// // // // // //   DesiredAnglePitch=0.10*(rx.ch[3]-1500);
// // // // // //   InputThrottle=rx.ch[3];
// // // // // //   DesiredRateYaw=0.15*(rx.ch[3]-1500);
  
// // // // //   DesiredAngleRoll  = 0.10 * (rx.ch[3] - 1500);
// // // // // DesiredAnglePitch = 0.10 * (rx.ch[2] - 1500);

// // // // // // Dung cho motor brushlless
// // // // // // InputThrottle = rx.ch[0];

// // // // // InputThrottle = map(rx.ch[0], 1000, 2000, 0, 255);
// // // // // InputThrottle = constrain(InputThrottle, 0, 255);

// // // // // DesiredRateYaw = 0.15 * (rx.ch[1] - 1500);

// // // // //   ErrorAngleRoll=DesiredAngleRoll-KalmanAngleRoll;
// // // // //   ErrorAnglePitch=DesiredAnglePitch-KalmanAnglePitch;
// // // // //   pid_equation(ErrorAngleRoll, PAngleRoll, IAngleRoll, DAngleRoll, PrevErrorAngleRoll, PrevItermAngleRoll);     
// // // // //   DesiredRateRoll=PIDReturn[0]; 
// // // // //   PrevErrorAngleRoll=PIDReturn[1];
// // // // //   PrevItermAngleRoll=PIDReturn[2];
// // // // //   pid_equation(ErrorAnglePitch, PAnglePitch, IAnglePitch, DAnglePitch, PrevErrorAnglePitch, PrevItermAnglePitch);
// // // // //   DesiredRatePitch=PIDReturn[0]; 
// // // // //   PrevErrorAnglePitch=PIDReturn[1];
// // // // //   PrevItermAnglePitch=PIDReturn[2];
// // // // //   ErrorRateRoll=DesiredRateRoll-RateRoll;
// // // // //   ErrorRatePitch=DesiredRatePitch-RatePitch;
// // // // //   ErrorRateYaw=DesiredRateYaw-RateYaw;
// // // // //   pid_equation(ErrorRateRoll, PRateRoll, IRateRoll, DRateRoll, PrevErrorRateRoll, PrevItermRateRoll);
// // // // //        InputRoll=PIDReturn[0];
// // // // //        PrevErrorRateRoll=PIDReturn[1]; 
// // // // //        PrevItermRateRoll=PIDReturn[2];
// // // // //   pid_equation(ErrorRatePitch, PRatePitch,IRatePitch, DRatePitch, PrevErrorRatePitch, PrevItermRatePitch);
// // // // //        InputPitch=PIDReturn[0]; 
// // // // //        PrevErrorRatePitch=PIDReturn[1]; 
// // // // //        PrevItermRatePitch=PIDReturn[2];
// // // // //   pid_equation(ErrorRateYaw, PRateYaw,IRateYaw, DRateYaw, PrevErrorRateYaw, PrevItermRateYaw);
// // // // //        InputYaw=PIDReturn[0]; 
// // // // //        PrevErrorRateYaw=PIDReturn[1]; 
// // // // //        PrevItermRateYaw=PIDReturn[2];

// // // // //   InputRoll  = constrain(InputRoll,  -60, 60);
// // // // //   InputPitch = constrain(InputPitch, -60, 60);
// // // // //   InputYaw   = constrain(InputYaw,   -40, 40);

// // // // //   // Dung cho motor brushlless
// // // // // //   if (InputThrottle > 1800) InputThrottle = 1800;
// // // // // //    MotorInput1= 1.024*(InputThrottle+InputRoll-InputPitch-InputYaw);
// // // // // //   MotorInput2= 1.024*(InputThrottle-InputRoll-InputPitch+InputYaw);
// // // // // //   MotorInput3= 1.024*(InputThrottle-InputRoll+InputPitch-InputYaw);
// // // // // //   MotorInput4= 1.024*(InputThrottle+InputRoll+InputPitch+InputYaw);


// // // // // //   MotorInput1= 1.024*(InputThrottle-InputRoll-InputPitch-InputYaw);
// // // // // //   MotorInput2= 1.024*(InputThrottle-InputRoll+InputPitch+InputYaw);
// // // // // //   MotorInput3= 1.024*(InputThrottle+InputRoll+InputPitch-InputYaw);
// // // // // //   MotorInput4= 1.024*(InputThrottle+InputRoll-InputPitch+InputYaw);


// // // // // //    MotorInput1= 1.024*(InputThrottle+InputRoll);
// // // // // //   MotorInput2= 1.024*(InputThrottle-InputRoll);
// // // // // //   MotorInput3= 1.024*(InputThrottle-InputRoll);
// // // // // //   MotorInput4= 1.024*(InputThrottle+InputRoll);
   

// // // // // //  MotorInput1= 1.024*(InputThrottle-InputPitch);
// // // // // //   MotorInput2= 1.024*(InputThrottle-InputPitch);
// // // // // //   MotorInput3= 1.024*(InputThrottle+InputPitch);
// // // // // //   MotorInput4= 1.024*(InputThrottle+InputPitch);


// // // // // //  MotorInput1= 1.024*(InputThrottle-InputYaw);
// // // // // //   MotorInput2= 1.024*(InputThrottle+InputYaw);
// // // // // //   MotorInput3= 1.024*(InputThrottle-InputYaw);
// // // // // //   MotorInput4= 1.024*(InputThrottle+InputYaw);

  



// // // // // MotorInput1 = InputThrottle + InputRoll - InputPitch - InputYaw;
// // // // // MotorInput2 = InputThrottle - InputRoll - InputPitch + InputYaw;
// // // // // MotorInput3 = InputThrottle - InputRoll + InputPitch - InputYaw;
// // // // // MotorInput4 = InputThrottle + InputRoll + InputPitch + InputYaw;


// // // // // MotorInput1 = constrain(MotorInput1,0,255);
// // // // // MotorInput2 = constrain(MotorInput2,0,255);
// // // // // MotorInput3 = constrain(MotorInput3,0,255);
// // // // // MotorInput4 = constrain(MotorInput4,0,255);

// // // // // if(rx.ch[0] < 1050)
// // // // // {
// // // // //     MotorInput1 = 0;
// // // // //     MotorInput2 = 0;
// // // // //     MotorInput3 = 0;
// // // // //     MotorInput4 = 0;
// // // // //     reset_pid();
// // // // // }

// // // // //   // analogWrite(M1, MotorInput1);
// // // // //   // analogWrite(M2, MotorInput2);
// // // // //   // analogWrite(M3, MotorInput3);
// // // // //   // analogWrite(M4, MotorInput4);



// // // // // //     // Dung cho motor brushlless
// // // // // //   if (MotorInput1 > 2000)MotorInput1 = 1999;
// // // // // //   if (MotorInput2 > 2000)MotorInput2 = 1999; 
// // // // // //   if (MotorInput3 > 2000)MotorInput3 = 1999; 
// // // // // //   if (MotorInput4 > 2000)MotorInput4 = 1999;
// // // // // //   int ThrottleIdle=1180;
// // // // // //   if (MotorInput1 < ThrottleIdle) MotorInput1 = ThrottleIdle;
// // // // // //   if (MotorInput2 < ThrottleIdle) MotorInput2 = ThrottleIdle;
// // // // // //   if (MotorInput3 < ThrottleIdle) MotorInput3 = ThrottleIdle;
// // // // // //   if (MotorInput4 < ThrottleIdle) MotorInput4 = ThrottleIdle;
// // // // // //   int ThrottleCutOff=1000;
// // // // // //   if (rx.ch[0] < 1050) {
// // // // // //     MotorInput1=ThrottleCutOff; 
// // // // // //     MotorInput2=ThrottleCutOff;
// // // // // //     MotorInput3=ThrottleCutOff; 
// // // // // //     MotorInput4=ThrottleCutOff;
// // // // // //     reset_pid();
// // // // // //   }

// // // // // //   Serial.print(rx.ch[0]);

// // // // // //   Serial.print(rx.ch[1]);
// // // // // //   Serial.print(rx.ch[2]);
// // // // // //   Serial.println(rx.ch[3]);


// // // // //     Serial.print(" M1:"); Serial.print(rx.ch[0]);
// // // // //   Serial.print(" M2:"); Serial.print(rx.ch[1]);
// // // // //   Serial.print(" M3:"); Serial.print(rx.ch[2]);
// // // // //   Serial.print(" M4:"); Serial.print(rx.ch[3]);
// // // // //   Serial.print(" M5:"); Serial.println(rx.ch[4]);
  


 

// // // // //   // Serial.print(" M1:"); Serial.print(MotorInput1);
// // // // //   // Serial.print(" M2:"); Serial.print(MotorInput2);
// // // // //   // Serial.print(" M3:"); Serial.print(MotorInput3);
// // // // //   // Serial.print(" M4:"); Serial.println(MotorInput4);




// // // // // //   analogWrite(1,MotorInput1);
// // // // // //   analogWrite(2,MotorInput2);
// // // // // //   analogWrite(3,MotorInput3); 
// // // // // //   analogWrite(4,MotorInput4);
// // // // // //   battery_voltage();
// // // // // //   CurrentConsumed=Current*1000*0.004/3600+CurrentConsumed;
// // // // // //   BatteryRemaining=(BatteryAtStart-CurrentConsumed)/BatteryDefault*100;
// // // // // //   if (BatteryRemaining<=30) digitalWrite(5, HIGH);
// // // // // //   else digitalWrite(5, LOW);
// // // // //   while (micros() - LoopTimer < 4000);
// // // // //   LoopTimer=micros();
// // // // // }


// // // // // #include <Arduino.h>
// // // // // #include <Wire.h>

// // // // // #define MPU6050_ADDR 0x68

// // // // // //================ RAW =================
// // // // // int16_t AccX, AccY, AccZ;
// // // // // int16_t GyroX, GyroY, GyroZ;
// // // // // int16_t Temp;

// // // // // //================ Converted =================
// // // // // float ax, ay, az;
// // // // // float gx, gy, gz;

// // // // // void initMPU6050()
// // // // // {
// // // // //     // Wake up
// // // // //     Wire.beginTransmission(MPU6050_ADDR);
// // // // //     Wire.write(0x6B);
// // // // //     Wire.write(0x00);
// // // // //     Wire.endTransmission(true);

// // // // //     delay(100);

// // // // //     // Digital Low Pass Filter
// // // // //     // 0x05 = 10Hz
// // // // //     Wire.beginTransmission(MPU6050_ADDR);
// // // // //     Wire.write(0x1A);
// // // // //     Wire.write(0x05);
// // // // //     Wire.endTransmission(true);

// // // // //     // Gyroscope ±500°/s
// // // // //     Wire.beginTransmission(MPU6050_ADDR);
// // // // //     Wire.write(0x1B);
// // // // //     Wire.write(0x08);
// // // // //     Wire.endTransmission(true);

// // // // //     // Accelerometer ±2g
// // // // //     Wire.beginTransmission(MPU6050_ADDR);
// // // // //     Wire.write(0x1C);
// // // // //     Wire.write(0x00);
// // // // //     Wire.endTransmission(true);
// // // // // }

// // // // // void readMPU()
// // // // // {
// // // // //     Wire.beginTransmission(MPU6050_ADDR);
// // // // //     Wire.write(0x3B);

// // // // //     if (Wire.endTransmission(false) != 0)
// // // // //         return;

// // // // //     if (Wire.requestFrom(MPU6050_ADDR, 14) != 14)
// // // // //         return;

// // // // //     AccX = (Wire.read() << 8) | Wire.read();
// // // // //     AccY = (Wire.read() << 8) | Wire.read();
// // // // //     AccZ = (Wire.read() << 8) | Wire.read();

// // // // //     Temp = (Wire.read() << 8) | Wire.read();

// // // // //     GyroX = (Wire.read() << 8) | Wire.read();
// // // // //     GyroY = (Wire.read() << 8) | Wire.read();
// // // // //     GyroZ = (Wire.read() << 8) | Wire.read();

// // // // //     //---------------- Convert ----------------

// // // // //     ax = AccX / 16384.0f;
// // // // //     ay = AccY / 16384.0f;
// // // // //     az = AccZ / 16384.0f;

// // // // //     // ±500°/s
// // // // //     gx = GyroX / 65.5f;
// // // // //     gy = GyroY / 65.5f;
// // // // //     gz = GyroZ / 65.5f;
// // // // // }

// // // // // void setup()
// // // // // {
// // // // //     Serial.begin(115200);

// // // // //     Wire.begin();
// // // // //     Wire.setClock(400000);

// // // // //     initMPU6050();

// // // // //     Serial.println("MPU6050 READY");
// // // // // }

// // // // // void loop()
// // // // // {
// // // // //     readMPU();

// // // // //     Serial.print("ACC(g)  ");
// // // // //     Serial.print(ax,4);
// // // // //     Serial.print("  ");
// // // // //     Serial.print(ay,4);
// // // // //     Serial.print("  ");
// // // // //     Serial.print(az,4);

// // // // //     Serial.print("    ");

// // // // //     Serial.print("GYRO(deg/s)  ");
// // // // //     Serial.print(gx,2);
// // // // //     Serial.print("  ");
// // // // //     Serial.print(gy,2);
// // // // //     Serial.print("  ");
// // // // //     Serial.print(gz,2);

// // // // //     Serial.println();

// // // // //     delay(10);
// // // // // }


// // // // // #include <Arduino.h>
// // // // // #include <Wire.h>

// // // // // #define MPU6050_ADDR 0x68

// // // // // float AccX, AccY, AccZ;

// // // // // void setup() {
// // // // //   Serial.begin(115200);

// // // // //   Wire.begin();
// // // // //   Wire.setClock(400000);

// // // // //   // Wake up MPU6050
// // // // //   Wire.beginTransmission(MPU6050_ADDR);
// // // // //   Wire.write(0x6B);
// // // // //   Wire.write(0);
// // // // //   Wire.endTransmission(true);

// // // // //   Serial.println("MPU6050 Axis Test");
// // // // //   Serial.println("----------------------------");
// // // // //   Serial.println("Nghieng theo tung huong de xem truc nao thay doi.");
// // // // // }

// // // // // void loop() {

// // // // //   Wire.beginTransmission(MPU6050_ADDR);
// // // // //   Wire.write(0x3B);
// // // // //   Wire.endTransmission(false);

// // // // //   Wire.requestFrom(MPU6050_ADDR, 6);

// // // // //   if (Wire.available() < 6) return;

// // // // //   int16_t ax = Wire.read() << 8 | Wire.read();
// // // // //   int16_t ay = Wire.read() << 8 | Wire.read();
// // // // //   int16_t az = Wire.read() << 8 | Wire.read();

// // // // //   AccX = ax / 16384.0;
// // // // //   AccY = ay / 16384.0;
// // // // //   AccZ = az / 16384.0;

// // // // //   Serial.printf("X = %6.2f g   ", AccX);
// // // // //   Serial.printf("Y = %6.2f g   ", AccY);
// // // // //   Serial.printf("Z = %6.2f g\n", AccZ);

// // // // //   delay(100);
// // // // // }