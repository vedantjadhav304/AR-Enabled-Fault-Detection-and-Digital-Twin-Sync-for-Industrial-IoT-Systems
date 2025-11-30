/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-firebase-realtime-database/
*********/
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// Network and Firebase credentials
#define WIFI_SSID "vedant"
#define WIFI_PASSWORD "admin180"
#define Web_API_KEY "AIzaSyDK2EtgtQ31Z7Tt0fKCa2UougMMHKV2Yzo"
#define DATABASE_URL "https://robo-database-a86a2-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "vedantjadhav9324@gmail.com"
#define USER_PASS "admin180"

// User function
void processData(AsyncResult &aResult);

// Authentication
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// Timer variables for sending data every 10 seconds
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds in milliseconds

// ---------- Your servo/error detection data ----------
#define SERVO_COUNT 6

float targetAngle[SERVO_COUNT] = {10,20,30,40,50,60};
float actualAngle[SERVO_COUNT] = {11,21,1000,40,52,59};   // example

// thresholds
float MAX_POSITION_ERROR = 4.0;
float MAX_NOISE_JUMP     = 10.0;
unsigned long STALL_TIME = 400;
unsigned long TIMEOUT    = 1500;

// memory
float lastActual[SERVO_COUNT] = {0};
unsigned long lastMove[SERVO_COUNT] = {0};
unsigned long cmdTime[SERVO_COUNT]  = {0};

// final error array (your request)
int errorCode[SERVO_COUNT] = {0};

// ERROR CODE DEFINITIONS
#define ERR_NONE        0
#define ERR_POS         1
#define ERR_STALL       2
#define ERR_TIMEOUT     3
#define ERR_NOISE       4
#define ERR_RANGE       5

// ---------- Error detection function ----------
void checkErrors() {
  unsigned long now = millis();

  for(int i = 0; i < SERVO_COUNT; i++) {

    float t = targetAngle[i];
    float a = actualAngle[i];
    float last = lastActual[i];

    float e = fabs(t - a);
    float delta = fabs(a - last);

    errorCode[i] = ERR_NONE;   // reset first

    // ---------- PRIORITY 1: OUT OF RANGE ----------
    if(a < 0 || a > 300) {
        errorCode[i] = ERR_RANGE;
    }

    // ---------- PRIORITY 2: NOISE SPIKE ----------
    else if(delta > MAX_NOISE_JUMP) {
        errorCode[i] = ERR_NOISE;
    }

    // ---------- PRIORITY 3: POSITION ERROR ----------
    else if(e > MAX_POSITION_ERROR) {
        errorCode[i] = ERR_POS;
    }

    // movement detection
    if(delta > 0.5) {
        lastMove[i] = now;
    }

    // ---------- PRIORITY 4: STALL ----------
    if((now - lastMove[i]) > STALL_TIME && e > MAX_POSITION_ERROR) {
        errorCode[i] = ERR_STALL;
    }

    // ---------- PRIORITY 5: TIMEOUT ----------
    if((now - cmdTime[i]) > TIMEOUT && e > MAX_POSITION_ERROR) {
        errorCode[i] = ERR_TIMEOUT;
    }

    lastActual[i] = a;
  }
}

// ---------- Helpers to convert arrays to string form ----------
String floatArrayToString(float arr[], int len) {
  String s = "[";
  for (int i = 0; i < len; i++) {
    s += String(arr[i], 3); // 3 decimal places
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

void setup(){
  Serial.begin(115200);

  // Connect to Wi-Fi (unchanged from example)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  
  // Configure SSL client
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  
  // Initialize Firebase (exactly as example)
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  Serial.println("Setup complete.");
}

void loop(){
  // Maintain authentication and async tasks
  app.loop();
  // Check if authentication is ready
  if (app.ready()){ 
    // Periodic data sending every sendInterval
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval){
      // Update the last send time
      lastSendTime = currentTime;

      // 1) compute errors
      checkErrors();

      // 2) convert arrays to strings (same format you requested)
      String sTarget = floatArrayToString(targetAngle, SERVO_COUNT);
      String sActual = floatArrayToString(actualAngle, SERVO_COUNT);
      String sErr    = intArrayToString(errorCode, SERVO_COUNT);

      // 3) send to Firebase using example pattern (paths you requested earlier)
      Database.set<String>(aClient, "/robot1/target",    sTarget, processData, "RTDB_Send_Target");
      Database.set<String>(aClient, "/robot1/actual",    sActual, processData, "RTDB_Send_Actual");
      Database.set<String>(aClient, "/robot1/errorCode", sErr,    processData, "RTDB_Send_Err");

      // Print locally too
      Serial.println("Sent to Firebase:");
      Serial.println(sTarget);
      Serial.println(sActual);
      Serial.println(sErr);
    }
  }
}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available())
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}