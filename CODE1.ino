#include "robotic_data.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// ---------- Wi-Fi and Firebase credentials ----------
#define WIFI_SSID "vedant"
#define WIFI_PASSWORD "admin180"
#define Web_API_KEY "AIzaSyDK2EtgtQ31Z7Tt0fKCa2UougMMHKV2Yzo"
#define DATABASE_URL "https://robo-database-a86a2-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "vedantjadhav9324@gmail.com"
#define USER_PASS "admin180"

// ---------- Firebase setup ----------
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

void processData(AsyncResult &aResult);

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// ---------- Control variables ----------
const unsigned long sendInterval = 1000;  // 1 second between uploads
unsigned long lastSendTime = 0;
int dataIndex = 0;

const int numRows = sizeof(roboticArmData) / sizeof(roboticArmData[0]);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[Robot 1] Connecting to Wi-Fi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWi-Fi connected.");

  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);

  initializeApp(aClient, app, getAuth(user_auth), processData, "R1_Auth");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  Serial.println("[Robot 1] Firebase initialized.");
}

void loop() {
  app.loop();

  if (!app.ready()) return;

  unsigned long currentTime = millis();
  if (currentTime - lastSendTime < sendInterval) return;
  lastSendTime = currentTime;

  if (dataIndex >= numRows) {
    dataIndex = 0;
    Serial.println("[Robot 1] Restarting data sequence...");
  }

  String dataString = roboticArmData[dataIndex];
  String path = "/robots/robot1/data/" + String(dataIndex);

  Database.set<String>(aClient, path, dataString, processData, "R1_Data");
  Serial.print("R1: ");
  Serial.println(dataString);

  dataIndex++;
}

void processData(AsyncResult &aResult) {
  if (aResult.isError()) {
    Serial.printf("[R1] ❌ Error: %s (code %d)\n",
                  aResult.error().message().c_str(),
                  aResult.error().code());
  }
  if (aResult.available()) {
    Serial.printf("[R1] ✅ Sent: %s\n", aResult.c_str());
  }
}
