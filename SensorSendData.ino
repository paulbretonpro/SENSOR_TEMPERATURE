#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include "DHT.h"
#include <ArduinoJson.h>

#define DHTPIN 2      // what digital pin we're connected to
#define DHTTYPE DHT11 // DHT 11

DHT dht(DHTPIN, DHTTYPE);

// Network credentials
const char *ssid = "Robot";
const char *password = "RobotVaHeThongThongMinh";

// Set web server port number to 80
ESP8266WebServer server(80);

// Set Hours server
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Job send data
unsigned long lastApiRequestTime = 0;
const unsigned long apiRequestInterval = 3600000;

// Array store failed send value
struct SensorData {
  float temperature;
  float humidity;
  String datetime;
};
SensorData backup[168]; // size equal one week
int numElement = 0;

void handleSync();
void handleGet();
void handleSend();
String getDatetime();
void addSensorToBackup();

void setup()
{
  // Start DHT11
  dht.begin();
  Serial.begin(115200);
  delay(10);
  Serial.println();

  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println();

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }

  // Routes for web server
  server.on("/api/sync", HTTP_GET, handleSync);
  server.on("/api/sensor", HTTP_GET, handleGet);

  server.begin(); // Actually start the server
  Serial.println("HTTP server started");
  timeClient.begin();
}

void loop()
{
  server.handleClient();

  unsigned long currentTime = millis();

  if (lastApiRequestTime == 0 or currentTime - lastApiRequestTime >= apiRequestInterval)
  {
    handleSend();
    lastApiRequestTime = currentTime;
  }
}

void handleGet()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  DynamicJsonDocument jsonDocument(200); // Use DynamicJsonDocument instead of StaticJsonDocument
  jsonDocument["temperature"] = t;
  jsonDocument["humidity"] = h;
  jsonDocument["datetime"] = getDatetime();

  String jsonResponse;
  serializeJson(jsonDocument, jsonResponse);

  server.send(200, "application/json", jsonResponse);
}

void handleSync()
{
  // Create a JSON document
  DynamicJsonDocument doc(1024);

  // Create an array in the JSON document
  JsonArray jsonArray = doc.createNestedArray("data");

  // Fill the array with sensor data
  for (int i = 0; i < numElement; i++) {
    JsonObject obj = jsonArray.createNestedObject();
    obj["temperature"] = backup[i].temperature;
    obj["humidity"] = backup[i].humidity;
    obj["datetime"] = backup[i].datetime;
  }

  // Serialize the JSON document to a string
  String response;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  numElement = 0;
}

void handleSend()
{
  HTTPClient http;
  WiFiClient client;

  http.begin(client, "http://192.168.50.179/api/sensor");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Create the POST request body
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  String requestData = "temperature=" + String(t, 6) + "&humidity=" + String(h, 6);

  int httpResponseCode = http.POST(requestData);

  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println("HTTP Response: " + response);
  }
  else
  {
    Serial.print("HTTP Error Code: ");
    Serial.println(httpResponseCode);
  }
  addSensorToBackup(t, h);

  http.end();
}

String getDatetime()
{
  String currentHourFormated = "";
  String currentMonthFormated = "";
  String currentDayFormated= "";
  
  timeClient.update();
  
  time_t epochTime = timeClient.getEpochTime();
  int currentHour = timeClient.getHours();
  if (currentHour < 10)
  {
    currentHourFormated = "0" + String(currentHour);
  }
  else
  {
    currentHourFormated = String(currentHour);
  }
  // Get a time structure
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  if (monthDay < 10)
  {
    currentDayFormated = "0" + String(monthDay);
  }
  else
  {
    currentDayFormated = String(monthDay);
  }
  int currentMonth = ptm->tm_mon + 1;
  if (currentMonth < 10)
  {
    currentMonthFormated = "0" + String(currentMonth);
  }
  else
  {
    currentMonthFormated = String(currentMonth);
  }
  int currentYear = ptm->tm_year + 1900;
  String currentDate = String(currentYear) + "-" + currentMonthFormated + "-" + currentDayFormated + "T" + currentHourFormated + ":00:00Z";

  return currentDate;
}

void addSensorToBackup(float temperature, float humidity) {
  if (numElement < 168) {
    backup[numElement].temperature = temperature;
    backup[numElement].humidity = humidity;
    backup[numElement].datetime = getDatetime();
    numElement++;
  } else {
    Serial.println("Backup is full, cannot add more data points.");
  }
}
