#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "DHT.h"

#define DHTPIN 2        // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

// Network credentials
const char* ssid = "Robot";
const char* password = "RobotVaHeThongThongMinh";

void setup() {
  // Start DHT11  
  dht.begin();  
  Serial.begin(115200);
  delay(10);
  Serial.println();
    
  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println();

  //Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    
    HTTPClient http;
    WiFiClient client;

    http.begin(client, "http://192.168.50.179/api/sensor");
    
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Create the POST request body
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    String requestData = "temperature=" + String(t, 6) + "&humidity=" + String(h, 6);
    
    int httpResponseCode = http.POST(requestData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response: " + response);
    } else {
      Serial.print("HTTP Error Code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
  
  delay(3600*1000); // Wait for a while before making the next request
}
