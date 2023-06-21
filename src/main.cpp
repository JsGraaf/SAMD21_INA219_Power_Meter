#include <Arduino.h>
#include <Adafruit_INA219.h>

#define QT_RESET_PIN 8
#define QT_USR_BTN 7
#define QT_ENABLE_PIN 9
#define QT_LDO2_PIN 2

#define ADC_COMP 0.013

Adafruit_INA219 INA219;

struct measureStruct {
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;
  float LDO2_v = 0;
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
  pinMode(QT_ENABLE_PIN, OUTPUT);
  pinMode(QT_LDO2_PIN, INPUT);
  
  /* Pulling high to prevent stuck in reset */
  digitalWrite(QT_RESET_PIN, HIGH);
  digitalWrite(QT_USR_BTN, HIGH);
  digitalWrite(QT_ENABLE_PIN, LOW);

  /* Starting and testing INA219 */
  if (!INA219.begin()) {
    Serial.write("f");
    while(1) {delay(10);}
  }
  /* Calibrate INA219 for 16v 400mA range */
  INA219.setCalibration_16V_400mA();
  /* Set ADC resolution to 12 bits */
  analogReadResolution(12);
}

void samplePower(measureStruct* sample) {
  float shuntvoltage = INA219.getShuntVoltage_mV();
  float busvoltage = INA219.getBusVoltage_V();
  sample->current_mA = INA219.getCurrent_mA();
  sample->power_mW = INA219.getPower_mW();
  sample->loadvoltage = busvoltage + (shuntvoltage / 1000);
  // Max level is 3.3v (logic high of XIAO), 12-bit res gives 4096
  sample->LDO2_v = analogRead(QT_LDO2_PIN) * 3.3 / 4096.0 - ADC_COMP;
}

void sendSample(measureStruct sample) {
  Serial.printf("%f:%f:%f:%f;", sample.loadvoltage, sample.current_mA, sample.power_mW, sample.LDO2_v);
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
      delay(1000);
      digitalWrite(QT_RESET_PIN, HIGH);
      delay(1000);
      digitalWrite(QT_USR_BTN, LOW);
      delay(1000);
      digitalWrite(QT_USR_BTN, HIGH);
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
      if (testDuration_ms == 0) {
        testDuration_ms = 10000;
      }
      if (timeBetween_ms == 0) {
        timeBetween_ms = 500;
      }
      /* Clear serial */
      clearSerialBuffer();
      /* Enable circuits */
      digitalWrite(QT_ENABLE_PIN, HIGH);
      performPowerTest(timeBetween_ms, testDuration_ms);
      digitalWrite(QT_ENABLE_PIN, LOW);
      Serial.write("END");
      break;
    }
    case 'y': { /* Continuous power testing */
      // Get 1 sample and print 1 sample per 500ms
      measureStruct sample;
      while (!Serial.available()) {
        samplePower(&sample);
        sendSample(sample);
        Serial.println("\n");
        delay(500);
      }
      clearSerialBuffer();
      break;
    }
    default:
      /* Flush Serial */
      Serial.write('e');
      break;
    }
  }
}