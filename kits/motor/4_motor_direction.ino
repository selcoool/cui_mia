#include <Arduino.h>
#include <vector>

struct MOTOR_PINS {
  int pinEn;
  int pinIN1;
  int pinIN2;
};

std::vector<MOTOR_PINS> motorPins = {
  {5, 18, 19},  // RIGHT_MOTOR: EnA, IN1, IN2
  {5, 21, 22},  // LEFT_MOTOR: EnB, IN3, IN4
};

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0

#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

#define FORWARD 1
#define BACKWARD -1

const int PWMFreq = 1000;  // 1 KHz
const int PWMResolution = 8;
const int PWMSpeedChannelA = 2;  // PWM cho động cơ phải
const int PWMSpeedChannelB = 3;  // PWM cho động cơ trái

void setup() {
  Serial.begin(115200);

  for (size_t i = 0; i < motorPins.size(); i++) {
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);
    ledcAttachPin(motorPins[i].pinEn, (i == 0) ? PWMSpeedChannelA : PWMSpeedChannelB);
    ledcSetup((i == 0) ? PWMSpeedChannelA : PWMSpeedChannelB, PWMFreq, PWMResolution);
  }
}

void rotateMotor(int motorNumber, int motorDirection, int speed = 200) {
  int pwmChannel = (motorNumber == RIGHT_MOTOR) ? PWMSpeedChannelA : PWMSpeedChannelB;

  if (motorDirection == FORWARD) {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  } else if (motorDirection == BACKWARD) {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);
  } else {  // Trường hợp dừng động cơ
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  }

  ledcWrite(pwmChannel, (motorDirection == STOP) ? 0 : speed);
}

void moveCar(int inputValue) {
  Serial.printf("Received command: %d\n", inputValue);
  switch (inputValue) {
    case UP:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);
      break;
    case DOWN:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);
      break;
    case LEFT:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);
      break;
    case RIGHT:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);
      break;
    case STOP:
    default:
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);
      break;
  }
}

void loop() {
  if (Serial.available()) {  // Kiểm tra nếu có dữ liệu từ Serial
    String command = Serial.readStringUntil('\n');  // Đọc toàn bộ lệnh
    command.trim();  // Xóa khoảng trắng thừa

    int commandInt = command.toInt();  // Chuyển thành số nguyên
    Serial.println(commandInt);  // In ra để kiểm tra

    moveCar(commandInt);
  }
}
