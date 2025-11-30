#include <Arduino.h>

#define SERVO_COUNT 6

float targetAngle[SERVO_COUNT];
float actualAngle[SERVO_COUNT];
float lastActual[SERVO_COUNT];

unsigned long lastStallTime[SERVO_COUNT] = {0};

float genAngle(int servo, float base) {
  float t = millis() / 500.0;
  return base + 20.0 * sin(t + servo);
}

// ---------------- CHECKSUM ----------------
uint8_t calcChecksum(const uint8_t *data, uint8_t len) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < len; i++) sum ^= data[i];
  return sum;
}

// ---------------- ERROR SIMULATION ----------------
// Randomly inject one of these:
//  1) position offset
//  2) noise spike
//  3) stall (freeze actualAngle)
void injectRandomError() {
  if (random(0, 100) >= 5) return;  
  // ~5% chance each packet (adjust as you want)

  int servo = random(0, SERVO_COUNT);  // pick a random joint
  int errType = random(1, 4);          // 1–3

  switch (errType) {
    case 1:  // Position error (target far from actual)
      actualAngle[servo] += random(-50, 50);  // huge error
      Serial.print("[ERR] Position error on servo ");
      Serial.println(servo);
      break;

    case 2:  // Noise spike
      actualAngle[servo] += random(-100, 100);
      Serial.print("[ERR] Noise spike on servo ");
      Serial.println(servo);
      break;

    case 3:  // Stall (freeze movement for a while)
      actualAngle[servo] = lastActual[servo];
      lastStallTime[servo] = millis();
      Serial.print("[ERR] Stall on servo ");
      Serial.println(servo);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
}

void loop() {

  // ---------------------------------------
  // 1. Generate smooth changing angles
  // ---------------------------------------
  for (int i = 0; i < SERVO_COUNT; i++) {
    targetAngle[i] = genAngle(i, 50);  
    actualAngle[i] = targetAngle[i] + random(-3, 3); 
  }

  // ---------------------------------------
  // 2. Apply random injected faults
  // ---------------------------------------
  injectRandomError();

  // Save last actual for stall simulation
  for (int i = 0; i < SERVO_COUNT; i++) {
    lastActual[i] = actualAngle[i];
  }

  // ---------------------------------------
  // 3. Build & send packet 0xAA + floats + checksum
  // ---------------------------------------
  uint8_t packet[1 + 48 + 1];
  uint8_t idx = 0;

  packet[idx++] = 0xAA;

  memcpy(&packet[idx], targetAngle, 24); idx += 24;
  memcpy(&packet[idx], actualAngle, 24); idx += 24;

  packet[idx] = calcChecksum(&packet[1], 48);

  Serial.write(packet, sizeof(packet));

  delay(5);  // ~200 Hz
}
