#include <Arduino.h>
#include <Adafruit_INA219.h>

#define QT_RESET_PIN 1
#define QT_USR_BTN 2

Adafruit_INA219 INA219;

struct measureStruct {
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
};

char input = '0';

void setup() {
  Serial.begin(115200);
  delay(2000);

  /* Configuring pins */
  pinMode(QT_RESET_PIN, OUTPUT);
  pinMode(QT_USR_BTN, OUTPUT);

  /* Starting and testing INA219 */
  if (!INA219.begin()) {
    Serial.println("Unable to find INA219");
    while(1) {delay(10);}
  }
  /* Calibrate INA219 for 16v 400mA range */
  INA219.setCalibration_16V_400mA();
  
  Serial.println("Power Meter and Controller online");
}

void samplePower(measureStruct* sample) {
  float shuntvoltage = INA219.getShuntVoltage_mV();
  float busvoltage = INA219.getBusVoltage_V();
  sample->current_mA = INA219.getCurrent_mA();
  sample->power_mW = INA219.getPower_mW();
  sample->loadvoltage = busvoltage + (shuntvoltage / 1000);
}

void sendSample(measureStruct sample) {
  Serial.printf("%f:%f:%f;", sample.current_mA, sample.loadvoltage, sample.power_mW);
}

void performPowerTest(int timeBetween_ms, int testDuration_ms) {
  measureStruct sample;
  // +1ms for conversion time of INA219
  int samples = (testDuration_ms/(timeBetween_ms + 1));
  for (int i=0; i<samples; i++) {
    samplePower(&sample);
    sendSample(sample);
  }
}

void loop() {
  if (Serial.available()) {
    input = Serial.read();
    /* Flush Serial */
    Serial.flush();
    Serial.print("Command Received: ");
    Serial.println(input);
    switch (input)
    {
    case 'r': { /* Code to reset the QT+ */
      Serial.println("Resetting the QT+");
      digitalWrite(QT_RESET_PIN, LOW);
      delay(250);
      digitalWrite(QT_RESET_PIN, HIGH);
      delay(1000);
      digitalWrite(QT_USR_BTN, LOW);
      delay(250);
      digitalWrite(QT_USR_BTN, HIGH);
      break;
    }
    case 'T': { /* Test connection */
      Serial.println("C");
      break;
    }
    case 'p': { /* Code to run power test */
      /* Get duration from the serial interface */
      int testDuration_ms = Serial.readStringUntil(' ').toInt();
      int timeBetween_ms = Serial.readStringUntil('\n').toInt();
      /* Clear serial */
      Serial.flush();
      /* Calculate amount of measurements */
      performPowerTest(timeBetween_ms, testDuration_ms);
      Serial.println("Done!");
      break;
    }
    default:
      Serial.println("Unkown command!");
      break;
    }
  }
}