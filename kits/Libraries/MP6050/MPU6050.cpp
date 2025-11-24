#include "MPU6050.h"

MPU6050::MPU6050(TwoWire &wirePort) {
    _wire = &wirePort;
}

bool MPU6050::begin() {
    // Kiểm tra WHO_AM_I
    _wire->beginTransmission(_address);
    if (_wire->endTransmission() != 0) return false;

    uint8_t id = readRegister(MPU6050_REG_WHO_AM_I);
    if (id != 0x68) return false;

    // Thức dậy MPU6050
    writeRegister(MPU6050_REG_PWR_MGMT_1, 0x00);
    delay(100);
    return true;
}

void MPU6050::reset() {
    writeRegister(MPU6050_REG_PWR_MGMT_1, 0x80);
    delay(100);
}

uint8_t MPU6050::readRegister(uint8_t reg) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission(false);
    _wire->requestFrom(_address, (uint8_t)1);
    if (_wire->available()) return _wire->read();
    return 0;
}

void MPU6050::readRegisters(uint8_t reg, uint8_t *buf, uint8_t len) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->endTransmission(false);
    _wire->requestFrom(_address, len);
    for (uint8_t i = 0; i < len && _wire->available(); i++) {
        buf[i] = _wire->read();
    }
}

void MPU6050::writeRegister(uint8_t reg, uint8_t data) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(data);
    _wire->endTransmission();
}

float MPU6050::convertAccel(int16_t raw) { return raw / 16384.0; }
float MPU6050::convertGyro(int16_t raw) { return raw / 131.0; }

void MPU6050::readAccelerometer(float &ax, float &ay, float &az) {
    uint8_t buf[6];
    readRegisters(MPU6050_REG_ACCEL_XOUT_H, buf, 6);
    int16_t rawX = (buf[0] << 8) | buf[1];
    int16_t rawY = (buf[2] << 8) | buf[3];
    int16_t rawZ = (buf[4] << 8) | buf[5];
    ax = convertAccel(rawX);
    ay = convertAccel(rawY);
    az = convertAccel(rawZ);
}

void MPU6050::readGyroscope(float &gx, float &gy, float &gz) {
    uint8_t buf[6];
    readRegisters(MPU6050_REG_GYRO_XOUT_H, buf, 6);
    int16_t rawX = (buf[0] << 8) | buf[1];
    int16_t rawY = (buf[2] << 8) | buf[3];
    int16_t rawZ = (buf[4] << 8) | buf[5];
    gx = convertGyro(rawX);
    gy = convertGyro(rawY);
    gz = convertGyro(rawZ);
}

float MPU6050::readTemperature() {
    uint8_t buf[2];
    readRegisters(MPU6050_REG_TEMP_OUT_H, buf, 2);
    int16_t rawTemp = (buf[0] << 8) | buf[1];
    return rawTemp / 340.0 + 36.53;
}
