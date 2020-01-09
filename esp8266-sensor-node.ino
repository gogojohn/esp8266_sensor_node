#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHT.h>
#include "WiFiAuth.h"

#define DHTTYPE DHT22
#define DHTPIN  4
#define LED 13

char MAC_ADDRESS[17];
ESP8266WebServer server(80);

// Initialize DHT sensor 
// NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an ESP8266,
// you need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// This is for the ESP8266 processor on ESP-01 
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266
 
float humidity, temp_c;             // Values read from sensor
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;   // will store last temp was read
const long interval = 2000;         // interval at which to read sensor


void getMeasurements() {
  /* Wait at least 2 seconds seconds between measurements.
     if the difference between the current time and last time you read
     the sensor is bigger than the interval you set, read the sensor
     Works better than delay for things happening elsewhere also
  */
  
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   
 
    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();      // Read humidity (percent)
    temp_c = dht.readTemperature();     // Read temperature as Celcius
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp_c)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}


void getWiFiMacAddress(void){
  /*
    Reads the device's Wi-Fi network interface controller (NIC) media access
    control (MAC) address, and constructs a string representation of it which is
    assigned to the MAC_ADDRESS global variable.
    
    The string is formatted as six groups of two zero-padded hexdecimal digits,
    separated by colons, in transmission order.
    
    example: 01:23:45:67:89:AB
  */

  byte mac[6];
  
  WiFi.macAddress(mac);
  sprintf(MAC_ADDRESS, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}


String getRSSI(void){
  /* Reads the RSSI, and returns it.
  */

  return String((int32_t)WiFi.RSSI());
}


void handleRoot() {
  /* Site root request handler: returns an HTTP response, bearing details
     for how to interact with the sensor node.
  */
  
  digitalWrite(LED, 1);
  String message = "<!DOCTYPE html>";
  message += "<html>";
  message += "<body>";
  message += "<h1>My First Heading</h1>";
  message += "<p>My first paragraph.</p>";
  message += "<script>alert('hello, you!');</script>";
  message += "</body>";
  message += "</html>";
  server.send(200, "text/html", message);
  digitalWrite(LED, 0);
}


void handleJson() {
  /* JSON request handler: returns an HTTP response, bearing a JSON encoded string, 
     and containing the following elements:
     
     (1) MAC address of sensor node
     (2) RSSI of sensor node
     (3) temperature measurement value, and units (in degrees Celcius)
     (4) relative humidity value, and units
  */
  
  digitalWrite(LED, 1);
  getMeasurements();
  String current_RSSI = getRSSI();
  String message = "{";
  message += "\"MAC address\": \"" + String(MAC_ADDRESS) + "\", ";
  message += "\"RSSI\": {\"value\":" + current_RSSI + ", \"units\":\"dBm\"},";
  message += "\"temperature\": {\"value\":" + String((int)temp_c) + ", \"units\":\"degrees C\"},";
  message += "\"relative humidity\": {\"value\":" + String((int)humidity) + ", \"units\":\"%\"}";
  message += "}";
  server.send(200, "application/json", message);
  digitalWrite(LED, 0);
}


void handleNotFound(){
  /* Not found request handler: returns an HTTP response, for URLs that do not exist
     (e.g. 404: NOT FOUND).
  */
  
  digitalWrite(LED, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(LED, 0);
}


void setup(void){
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
  Serial.begin(115200); 
  WiFi.begin(SSID, PASSWORD);
  getWiFiMacAddress();
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to: ");
  Serial.println(SSID);
  Serial.print("MAC address: ");
  Serial.println(MAC_ADDRESS);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // Registers the site root handler.
  server.on("/", handleRoot);

  // Registers the JSON handler.
  server.on("/json", handleJson);

  // Registers the not found (404) handler.
  server.onNotFound(handleNotFound);

  // Starts the HTTP server.
  server.begin();
  Serial.println("HTTP server started");
}


void loop(void){
  server.handleClient();
}
