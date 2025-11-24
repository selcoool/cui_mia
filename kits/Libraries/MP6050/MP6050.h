#pragma once
#include <Wire.h>
#include <Arduino.h>

class MPU6050 {
public:
    MPU6050(TwoWire &wirePort = Wire);

    // Khởi tạo MPU6050, trả về true nếu OK
    bool begin();

    // Đọc gia tốc (g)
    void readAccelerometer(float &ax, float &ay, float &az);

    // Đọc gyro (deg/s)
    void readGyroscope(float &gx, float &gy, float &gz);

    // Đọc nhiệt độ (°C)
    float readTemperature();

    // Reset MPU6050
    void reset();

private:
    TwoWire *_wire;
    const uint8_t _address = 0x68; // Địa chỉ cố định

    uint8_t readRegister(uint8_t reg);
    void readRegisters(uint8_t reg, uint8_t *buf, uint8_t len);
    void writeRegister(uint8_t reg, uint8_t data);

    float convertAccel(int16_t raw);
    float convertGyro(int16_t raw);
};

// Thanh ghi
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_TEMP_OUT_H   0x41
#define MPU6050_REG_GYRO_XOUT_H  0x43
#define MPU6050_REG_WHO_AM_I     0x75
