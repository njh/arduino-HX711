#include "HX711_asukiaaa.h"

#define HX711_VBG 1.25f
#define HX711_AVDD 4.2987f  // (HX711_VBG*((HX711_R1+HX711_R2)/HX711_R2))
#define HX711_ADC1BIT (HX711_AVDD / 16777216) // 16777216=(2^24)
#define HX711_PGA 128

namespace HX711_asukiaaa {
  Parser::Parser(float ratedVoltage, float ratedGram, float r1, float r2):
    // ratedVoltage(ratedVoltage),
    // ratedGram(ratedGram),
    // r1(r1),
    // r2(r2),
    avdd(HX711_VBG * ((r1 + r2) / r2)),
    adc1bit(avdd / 16777216),
    scale(ratedVoltage * HX711_AVDD / ratedGram * HX711_PGA) {
    // offsetGram = 0;
  }

  float Parser::parseToGram(int32_t raw) {
    return (float) raw * adc1bit / scale; // - offsetGram;
  }

  Reader::Reader(int* pinsDout, int doutLen, int pinSlk):
    doutLen(doutLen),
    pinsDout(pinsDout),
    pinSlk(pinSlk) {
    values = new int32_t[doutLen];
  }

  Reader::~Reader() {
    delete[] values;
  }

  void Reader::begin() {
    pinMode(pinSlk, OUTPUT);
    for (int i = 0; i < doutLen; ++i) {
      pinMode(pinsDout[i], INPUT);
    }
    reset();
  }

  void Reader::reset() {
    digitalWrite(pinSlk, HIGH);
    delayMicroseconds(100);
    digitalWrite(pinSlk, LOW);
    delayMicroseconds(100);
  }

  int Reader::read(int sumNumber) {
    uint64_t sumValues[doutLen];
    for (int i = 0; i < doutLen; ++i) {
      sumValues[i] = 0;
    }
    int32_t data[doutLen];
    // Serial.println("readLen " + String(readLen));
    for (int sumCount = 0; sumCount < sumNumber; ++sumCount) {
      int result = readRawOnce(data);
      if (result != 0) { return result; }
      for (int i = 0; i < doutLen; ++i) {
        // Serial.println("data " + String(sumCount) + " " + String(i) + " " + String(data[i]));
        sumValues[i] += data[i];
      }
    }
    for (int i = 0; i < doutLen; ++i) {
      values[i] = sumValues[i] / sumNumber;
    }
    return 0;
  }

  int Reader::readRawOnce(int32_t *readValues, unsigned long timeout) {
    int readLen = doutLen;
    for (int i = 0; i < readLen; ++i) {
      readValues[i] = 0;
    }
    unsigned long startedAt = millis();
    while (!pinsAreReady()) {
      if (millis() - startedAt > timeout) {
        return Error::timeout;
      }
    };
    delayMicroseconds(10);
    for (int clockCount = 0; clockCount < 24; clockCount++) {
      digitalWrite(pinSlk, 1);
      delayMicroseconds(5);
      digitalWrite(pinSlk, 0);
      delayMicroseconds(5);
      for (int i = 0; i < readLen; ++i) {
        readValues[i] = (readValues[i] << 1) | (digitalRead(pinsDout[i]));
      }
    }
    digitalWrite(pinSlk, 1);
    delayMicroseconds(10);
    digitalWrite(pinSlk, 0);
    delayMicroseconds(10);
    // Serial.println("read");
    // Serial.println(readLen);
    for (int i = 0; i < readLen; ++i) {
      // Serial.println(readValues[i]);
      readValues[i] ^= 0x800000;
    }
    return 0;
  }

  bool Reader::pinsAreReady() {
    bool allReady = true;
    for (int i = 0; i < doutLen; ++i) {
      if (digitalRead(pinsDout[i]) == HIGH) {
        // Serial.println("notReady " + String(pinsDout[i]));
        allReady = false;
      }
    }
    return allReady;
  }
}
