#include <ESP8266WiFi.h>
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

// Job send data
unsigned long lastApiRequestTime = 0;
const unsigned long apiRequestInterval = 3600000;

void handleSync();
void handleGet();
void handleSend();

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
}

void loop()
{
  server.handleClient();

  unsigned long currentTime = millis();
  
  if (lastApiRequestTime == 0 or currentTime - lastApiRequestTime >= apiRequestInterval) {
    handleSend();
    lastApiRequestTime = currentTime;
  }
}

void handleGet()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  DynamicJsonDocument jsonDocument(200);  // Use DynamicJsonDocument instead of StaticJsonDocument
  jsonDocument["temperature"] = t;
  jsonDocument["humidity"] = h;

  String jsonResponse;
  serializeJson(jsonDocument, jsonResponse);
  
  server.send(200, "application/json", jsonResponse);
}

void handleSync()
{
  server.send(200, "text/plain", "Hello world!"); // Send HTTP status 200 (Ok) and send some text to the browser/client
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

  http.end();
  
}
