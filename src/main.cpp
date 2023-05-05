#include <Arduino.h>
#include <Adafruit_INA219.h>

#define QT_RESET_PIN 8
#define QT_USR_BTN 7

Adafruit_INA219 INA219;

struct measureStruct {
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
};

char input = '0';

void clearSerialBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}

void setup() {
  Serial.begin(115200);
  clearSerialBuffer();
  delay(2000);

  /* Configuring pins */
  pinMode(QT_RESET_PIN, OUTPUT);
  pinMode(QT_USR_BTN, OUTPUT);

  /* Starting and testing INA219 */
  if (!INA219.begin()) {
    Serial.write("f");
    while(1) {delay(10);}
  }
  /* Calibrate INA219 for 16v 400mA range */
  INA219.setCalibration_16V_400mA();
}

void samplePower(measureStruct* sample) {
  float shuntvoltage = INA219.getShuntVoltage_mV();
  float busvoltage = INA219.getBusVoltage_V();
  sample->current_mA = INA219.getCurrent_mA();
  sample->power_mW = INA219.getPower_mW();
  sample->loadvoltage = busvoltage + (shuntvoltage / 1000);
}

void sendSample(measureStruct sample) {
  Serial.printf("%f:%f:%f;", sample.loadvoltage, sample.current_mA, sample.power_mW);
}

void performPowerTest(int timeBetween_ms, int testDuration_ms) {
  measureStruct sample;
  // Calculate amount of samples
  int samples = (testDuration_ms/timeBetween_ms);
  for (int i=0; i<samples; i++) {
    samplePower(&sample);
    sendSample(sample);
    delay(max(timeBetween_ms-1, 0));
  }
}

void loop() {
  if (Serial.available()) {
    input = Serial.read();
    Serial.write('c');
    switch (input)
    {
    case 'r': { /* Code to reset the QT+ */
      digitalWrite(QT_RESET_PIN, LOW);
      delay(250);
      digitalWrite(QT_RESET_PIN, HIGH);
      delay(1000);
      digitalWrite(QT_USR_BTN, LOW);
      delay(250);
      digitalWrite(QT_USR_BTN, HIGH);
      /* Flush Serial */
      Serial.write('d');
      break;
    }
    case 't': { /* Test connection */
      /* Flush Serial */
      Serial.write('c');
      break;
    }
    case 'p': { /* Code to run power test */
      /* Get duration from the serial interface */
      int testDuration_ms = Serial.parseInt();
      int timeBetween_ms = Serial.parseInt();
      /* Clear serial */
      clearSerialBuffer();
      /* Calculate amount of measurements */
      performPowerTest(timeBetween_ms, testDuration_ms);
      Serial.write("END");
      break;
    }
    default:
      /* Flush Serial */
      Serial.write('e');
      break;
    }
  }
}