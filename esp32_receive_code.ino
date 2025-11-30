/********* Fixed: UART float arrays -> Firebase *********/
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// ---------------- WIFI + FIREBASE ----------------
#define WIFI_SSID "vedant"
#define WIFI_PASSWORD "admin180"
#define Web_API_KEY "AIzaSyDK2EtgtQ31Z7Tt0fKCa2UougMMHKV2Yzo"
#define DATABASE_URL "https://robo-database-a86a2-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "vedantjadhav9324@gmail.com"
#define USER_PASS "admin180"

// Firebase plumbing
void processData(AsyncResult &aResult);
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
FirebaseApp app;

WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

RealtimeDatabase Database;

// ---------------- PARAMETERS ----------------
#define SERVO_COUNT 6

float targetAngle[SERVO_COUNT];
float actualAngle[SERVO_COUNT];

float MAX_POSITION_ERROR = 4.0;
float MAX_NOISE_JUMP = 10.0;

unsigned long STALL_TIME = 400;
unsigned long TIMEOUT = 1500;

float lastActual[SERVO_COUNT] = {0};
unsigned long lastMove[SERVO_COUNT] = {0};
unsigned long cmdTime[SERVO_COUNT] = {0};

int errorCode[SERVO_COUNT] = {0};

// ERROR CONSTANTS
#define ERR_NONE   0
#define ERR_POS    1
#define ERR_STALL  2
#define ERR_TIMEOUT 3
#define ERR_NOISE  4
#define ERR_RANGE  5

// ---------------- TIMING ----------------
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 500; // 0.5s

// ---------------- UART / PACKET ----------------
const uint8_t PACKET_HEADER = 0xAA;

const size_t FLOATS_PER_ARR = 6;
const size_t BYTES_PER_FLOAT = 4;

const size_t PAYLOAD_BYTES = FLOATS_PER_ARR * BYTES_PER_FLOAT * 2; // 48
const size_t PACKET_TOTAL_BYTES = 1 + PAYLOAD_BYTES + 1; // header + payload + checksum = 50

// ---------------- HELPERS ----------------
uint8_t calcChecksum(const uint8_t *data, uint8_t len) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < len; i++) sum ^= data[i];
  return sum;
}

String floatArrayToString(float arr[], int len) {
  String s = "[";
  for (int i = 0; i < len; i++) {
    s += String(arr[i], 3);
    if (i < len - 1) s += ", ";
  }
  s += "]";
  return s;
}

String intArrayToString(int arr[], int len) {
  String s = "[";
  for (int i = 0; i < len; i++) {
    s += String(arr[i]);
    if (i < len - 1) s += ", ";
  }
  s += "]";
  return s;
}

// ---------------- UART READ (non-blocking) ----------------
bool uartTryReadPacketAndParse() {

  if (Serial1.available() < (int)PACKET_TOTAL_BYTES) return false;

  int next = Serial1.peek();
  if (next == -1) return false;

  if ((uint8_t)next != PACKET_HEADER) {
    Serial1.read();
    return false;
  }

  // Valid header → consume
  Serial1.read();

  uint8_t payload[PAYLOAD_BYTES];
  size_t got = Serial1.readBytes((char*)payload, PAYLOAD_BYTES);
  if (got != PAYLOAD_BYTES) return false;

  uint8_t checksumByte = 0;
  if (Serial1.readBytes((char*)&checksumByte, 1) != 1) return false;

  if (checksumByte != calcChecksum(payload, PAYLOAD_BYTES)) {
    Serial.println("UART checksum error");
    return false;
  }

  memcpy(targetAngle, payload, 24);
  memcpy(actualAngle, payload + 24, 24);

  unsigned long now = millis();

  for (int i = 0; i < SERVO_COUNT; ++i) {
    cmdTime[i] = now;
    lastMove[i] = now;
    if (lastActual[i] == 0.0f)
      lastActual[i] = actualAngle[i];
  }

  return true;
}

// ---------------- ERROR CHECK SYSTEM ----------------
void checkErrors() {
  unsigned long now = millis();

  for (int i = 0; i < SERVO_COUNT; ++i) {

    float t = targetAngle[i];
    float a = actualAngle[i];
    float last = lastActual[i];

    float e = fabs(t - a);
    float delta = fabs(a - last);

    int code = ERR_NONE;

    if (a < 0.0f || a > 300.0f)
      code = ERR_RANGE;
    else if (delta > MAX_NOISE_JUMP)
      code = ERR_NOISE;
    else if (e > MAX_POSITION_ERROR)
      code = ERR_POS;

    if (delta > 0.5f)
      lastMove[i] = now;

    if ((now - lastMove[i]) > STALL_TIME && e > MAX_POSITION_ERROR)
      code = ERR_STALL;

    if ((now - cmdTime[i]) > TIMEOUT && e > MAX_POSITION_ERROR)
      code = ERR_TIMEOUT;

    errorCode[i] = code;

    lastActual[i] = a;
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  Serial1.begin(115200, SERIAL_8N1, 4, 5);
  Serial.println("UART initialized (RX=4, TX=5)");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();

  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);

  initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");

  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  Serial.println("Setup complete.");
}

// ---------------- LOOP ----------------
void loop() {

  app.loop();

  if (uartTryReadPacketAndParse()) {
    Serial.print("Got target: ");
    for (int i = 0; i < SERVO_COUNT; ++i) {
      Serial.print(targetAngle[i], 3);
      Serial.print(" ");
    }
    Serial.print(" | actual: ");
    for (int i = 0; i < SERVO_COUNT; ++i) {
      Serial.print(actualAngle[i], 3);
      Serial.print(" ");
    }
    Serial.println();
  }

  // Update error detection
  checkErrors();

  unsigned long now = millis();

  if (app.ready() && (now - lastSendTime >= sendInterval)) {

    lastSendTime = now;

    String sTarget = floatArrayToString(targetAngle, SERVO_COUNT);
    String sActual = floatArrayToString(actualAngle, SERVO_COUNT);
    String sErr = intArrayToString(errorCode, SERVO_COUNT);

    Database.set<String>(aClient, "/robot1/target", sTarget, processData, "RTDB_Send_Target");
    Database.set<String>(aClient, "/robot1/actual", sActual, processData, "RTDB_Send_Actual");
    Database.set<String>(aClient, "/robot1/errorCode", sErr, processData, "RTDB_Send_Err");

    Serial.println("Sent to Firebase:");
    Serial.println(sTarget);
    Serial.println(sActual);
    Serial.println(sErr);
  }
}

// ---------------- CALLBACK ----------------
void processData(AsyncResult &aResult) {
  if (!aResult.isResult()) return;

  if (aResult.isEvent())
    Serial.printf("Event: %s, code: %d\n",
                  aResult.eventLog().message().c_str(),
                  aResult.eventLog().code());

  if (aResult.isError())
    Serial.printf("Error: %s, code: %d\n",
                  aResult.error().message().c_str(),
                  aResult.error().code());

  if (aResult.available())
    Serial.printf("Task: %s, payload: %s\n",
                  aResult.uid().c_str(),
                  aResult.c_str());
}
