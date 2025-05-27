#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// Raw values
int16_t ax, ay, az, gx, gy, gz;

// Tính toán
float AccX, AccY, AccZ;
float RateRoll, RatePitch;
float AngleRoll, AnglePitch;

// Kalman filter
float KalmanAngleRoll = 0, KalmanUncertaintyRoll = 4;
float KalmanAnglePitch = 0, KalmanUncertaintyPitch = 4;

float t = 0.004; // chu kỳ tính toán 4ms
unsigned long prevTime = 0;

void kalman_1d(float& angle, float& uncertainty, float rate, float measured) {
  angle = angle + t * rate;
  uncertainty = uncertainty + t * t * 16; // variance = 4^2
  float kalmanGain = uncertainty / (uncertainty + 9); // measurement noise variance = 3^2
  angle = angle + kalmanGain * (measured - angle);
  uncertainty = (1 - kalmanGain) * uncertainty;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
}

void loop() {
  if (millis() - prevTime >= 20) { // 50 Hz
    prevTime = millis();

    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Convert to physical units
    AccX = ax / 16384.0;
    AccY = ay / 16384.0;
    AccZ = az / 16384.0;

    RateRoll = gx / 131.0;
    RatePitch = gy / 131.0;

    // Calculate angles from accelerometer
    AngleRoll = atan2(AccY, sqrt(AccX * AccX + AccZ * AccZ)) * 57.2958;
    AnglePitch = -atan2(AccX, sqrt(AccY * AccY + AccZ * AccZ)) * 57.2958;

    // Kalman filter
    kalman_1d(KalmanAngleRoll, KalmanUncertaintyRoll, RateRoll, AngleRoll);
    kalman_1d(KalmanAnglePitch, KalmanUncertaintyPitch, RatePitch, AnglePitch);

    // In ra Serial Plotter (chỉ số, không chuỗi)
    Serial.print(KalmanAngleRoll);
    Serial.print("\t");
    Serial.println(KalmanAnglePitch);
  }
}