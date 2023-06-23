// Joaquin R , Santi M , Liam P , Jony L

#include <Preferences.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "time.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "ORT-IoT"
#define WIFI_PASSWORD "OrtIOTnew22$2"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAkqGgK5yty417Luf4EASFMlPHDv3SZHDM"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "lauti.ridel@gmail.com"
#define USER_PASSWORD "dipidipi"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://tp5-firebase-default-rtdb.firebaseio.com/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;
long timerDelay = 30000; //30 segs
const char* ntpServer = "pool.ntp.org";

//TEMPERATURA

#include "DHT.h"

#define DHTPIN 23
#define DHTTYPE DHT11
#define TEMP 27

#define LED 25
#define BOTON1 34
#define BOTON2 35


DHT dht(DHTPIN, DHTTYPE);
Preferences preferences;

float temperaturaActual;
int funcionamiento;
float temperaturaUmbral = 26;
float tempVieja;

int lecturaBoton1;
int lecturaBoton2;

int estadoBoton;
int estadoBoton1;
int estadoBoton2;
int estadoBoton3;
int estadoPantalla;
int flagTemp;
unsigned long sendDataPrevMillis = 0;

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}


void setup() {
  preferences.begin("my-app", false);
  pinMode(LED, OUTPUT);
  pinMode(BOTON1, INPUT_PULLUP);
  pinMode(BOTON2, INPUT_PULLUP);

  pinMode(TEMP, INPUT);

  Serial.begin(115200);
  dht.begin();


  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);

  initWiFi();
  configTime(0, 0, ntpServer);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}


void loop() {


  tempVieja = temperaturaActual;
  temperaturaActual = dht.readTemperature();

  if (flagTemp == 0) {
    tempVieja = temperaturaActual;
    flagTemp = 1;
  }

  lecturaBoton1 = digitalRead(BOTON1);
  lecturaBoton2 = digitalRead(BOTON2);

  switch (funcionamiento) {
    case 0:
      {
        if (tempVieja != temperaturaActual) {
          display.clearDisplay();

        }
        display.setCursor(0, 25);
        display.print("Temp actual: ");
        display.println(temperaturaActual);
        display.display();

        unsigned int tempFlash = preferences.getUInt("tempFlash", 0);

        display.setCursor(0, 45);
        display.print("Temp umbral: ");
        display.print(temperaturaUmbral);
        display.display();

        if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
          //Serial.println ("Entre en el if ");
          sendDataPrevMillis = millis();

          //Get current timestamp
          timestamp = getTime();
          Serial.print ("time: ");
          Serial.println (timestamp);

          parentPath = databasePath + "/" + String(timestamp);

          /*json.set(tempPath.c_str(), String(bme.readTemperature()));
            json.set(humPath.c_str(), String(bme.readHumidity()));
            json.set(presPath.c_str(), String(bme.readPressure()/100.0F));*/
          json.set(tempPath.c_str(), temperaturaActual);
          json.set(timePath, String(timestamp));
          Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
        }

        if (lecturaBoton1 == LOW && lecturaBoton2 == LOW ) {
          estadoBoton3 = HIGH;
        }

        if (lecturaBoton1 == HIGH && lecturaBoton2 == HIGH && estadoBoton3 == HIGH) {
          estadoBoton3 = LOW;
          funcionamiento = 1;
        }

        break;
      }
    case 1:
      {
        display.clearDisplay();

        display.setCursor(0, 25);
        display.println("Ciclo de enviado");
        display.print(timerDelay/1000);
        display.print(" segundos");
        display.display();


        if (lecturaBoton1 == LOW && lecturaBoton2 == LOW ) {
          estadoBoton3 = HIGH;
        }

        if (lecturaBoton1 == HIGH && lecturaBoton2 == HIGH && estadoBoton3 == HIGH) {
          estadoBoton3 = LOW;
          //preferences.putUInt("tempFlash", temperaturaUmbral);
          funcionamiento = 0;
          display.clearDisplay();
          break;

        }

        if (lecturaBoton1 == LOW && lecturaBoton2 == HIGH && estadoBoton3 == LOW) {
          estadoBoton1 = HIGH;
        }

        if (lecturaBoton1 == HIGH && estadoBoton1 == HIGH && estadoBoton3 == LOW) {
          timerDelay -= 30000;
          estadoBoton1 = LOW;
        }

        if (lecturaBoton2 == LOW && lecturaBoton1 == HIGH && estadoBoton3 == LOW) {
          estadoBoton2 = HIGH;
        }

        if (lecturaBoton2 == HIGH && estadoBoton2 == HIGH && estadoBoton3 == LOW) {
          timerDelay += 30000;
          estadoBoton2 = LOW;
        }




        break;

      }

  }
}
