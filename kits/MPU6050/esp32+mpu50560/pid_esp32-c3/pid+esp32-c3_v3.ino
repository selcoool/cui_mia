#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// Motor pins
// Định nghĩa chân PWM cho động cơ
int motorPin1 = 2; // Chân PWM cho động cơ 1
int motorPin2 = 3; // Chân PWM cho động cơ 2
int motorPin3 = 6; // Chân PWM cho động cơ 3
int motorPin4 = 7; // Chân PWM cho động cơ 4

// PID parameters
float Kp_pitch = 5.0, Ki_pitch = 0.0, Kd_pitch = 5.0;
float Kp_roll = 5.0, Ki_roll = 0.0, Kd_roll = 5.0;

float previousErrorPitch = 0, integralPitch = 0;
float previousErrorRoll = 0, integralRoll = 0;

// MPU6050 variables
int16_t ax, ay, az;
int16_t gx, gy, gz;
float pitch, roll;

// Kalman filter variables
float KalmanPitch = 0.0, KalmanPitchUncertainty = 2.0;
float KalmanRoll  = 0.0, KalmanRollUncertainty  = 2.0;
float t = 0.1; // time step in seconds

void kalman_1d(float& KalmanState, float& KalmanUncertainty, float KalmanInput, float KalmanMeasurement) {
  KalmanState = KalmanState + (t * KalmanInput);
  KalmanUncertainty = KalmanUncertainty + (t * t * 4 * 4);  // motion noise variance = 16
  float KalmanGain = KalmanUncertainty / (KalmanUncertainty + 9); // measurement noise = 9 (3^2)
  KalmanState = KalmanState + KalmanGain * (KalmanMeasurement - KalmanState);
  KalmanUncertainty = (1 - KalmanGain) * KalmanUncertainty;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(4, 5); // SDA, SCL

  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful.");
  } else {
    Serial.println("MPU6050 connection failed.");
  }

  // PWM setup
  ledcSetup(0, 1000, 8);
  ledcSetup(1, 1000, 8);
  ledcSetup(2, 1000, 8);
  ledcSetup(3, 1000, 8);

  ledcAttachPin(motorPin1, 0);
  ledcAttachPin(motorPin2, 1);
  ledcAttachPin(motorPin3, 2);
  ledcAttachPin(motorPin4, 3);
}

void loop() {
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);

  // Raw angle estimates
  float rawPitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  float rawRoll  = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  // Apply Kalman filter
  kalman_1d(KalmanPitch, KalmanPitchUncertainty, gx / 131.0, rawPitch); // Convert gyro from raw to deg/s
  kalman_1d(KalmanRoll, KalmanRollUncertainty, gy / 131.0, rawRoll);

  // PID control
  float errorPitch = 0 - KalmanPitch;
  integralPitch += errorPitch;
  float derivativePitch = errorPitch - previousErrorPitch;
  float pidOutputPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
  previousErrorPitch = errorPitch;

  float errorRoll = 0 - KalmanRoll;
  integralRoll += errorRoll;
  float derivativeRoll = errorRoll - previousErrorRoll;
  float pidOutputRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * derivativeRoll;
  previousErrorRoll = errorRoll;

  int motorSpeed1 = constrain(128 + pidOutputPitch + pidOutputRoll, 0, 255);
  int motorSpeed2 = constrain(128 - pidOutputPitch + pidOutputRoll, 0, 255);
  int motorSpeed3 = constrain(128 + pidOutputPitch - pidOutputRoll, 0, 255);
  int motorSpeed4 = constrain(128 - pidOutputPitch - pidOutputRoll, 0, 255);

  // Output to Serial Plotter
  Serial.print("Pitch:"); Serial.print(KalmanPitch); Serial.print("\t");
  Serial.print("Roll:"); Serial.print(KalmanRoll); Serial.print("\t");
  Serial.print("Motor1:"); Serial.print(motorSpeed1); Serial.print("\t");
  Serial.print("Motor2:"); Serial.print(motorSpeed2); Serial.print("\t");
  Serial.print("Motor3:"); Serial.print(motorSpeed3); Serial.print("\t");
  Serial.print("Motor4:"); Serial.println(motorSpeed4);

  // Write to motors
  ledcWrite(0, motorSpeed1);
  ledcWrite(1, motorSpeed2);
  ledcWrite(2, motorSpeed3);
  ledcWrite(3, motorSpeed4);

  delay(100); // adjust for smoother plot
}
