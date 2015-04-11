#include <Wire.h> //I2C needed for sensors
#include "MPL3115A2.h" //Pressure sensor
#include "HTU21D.h" //Humidity sensor

// Includes for WiFi Connectivity
#include <SPI.h>
#include <WiFi.h>

MPL3115A2 myPressure; //Create an instance of the pressure sensor
HTU21D myHumidity; //Create an instance of the humidity sensor

//Hardware pin definitions for weather shield
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// digital I/O pins
#define STAT2 8

// analog I/O pins
#define REFERENCE_3V3 A3
#define LIGHT A1
#define BATT A2
#define WDIR A0
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


char ssid[] = "5at70m5G";      //  your network SSID (name) 
char pass[] = "Sarah0812!";   // your network password

int status = WL_IDLE_STATUS;

// Initialize the Wifi client library
WiFiClient client;

// Remote Server for posting temperature data
#define server "datadrop.wolframcloud.com"

// Debug variable
#define debug false

unsigned long lastConnectionTime;

float humidity = 0; // [%]
float tempf = 0; // [temperature F]
float pressure = 0;

float light_lvl = 455; //[analog value from 0 to 1023]

void setup()
{
  int i;
  
  while ( status != WL_CONNECTED) { 
    if (debug) {
      Serial.begin(115200);
      Serial.println();
      Serial.println("Connecting to WiFi...");
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
    }
    
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  } 
  
  if (debug)
  {
    printWifiStatus();
  }
  
  pinMode(STAT2, OUTPUT); //Status LED Green
  
  pinMode(REFERENCE_3V3, INPUT);
  pinMode(LIGHT, INPUT);

  //Configure the pressure sensor
  myPressure.begin(); // Get sensor online
  myPressure.setModeBarometer(); // Measure pressure in Pascals from 20 to 110 kPa
  myPressure.setOversampleRate(7); // Set Oversample to the recommended 128
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags 

  //Configure the humidity sensor
  myHumidity.begin();

  lastConnectionTime = millis();

  if (debug) {
    Serial.println();
    Serial.println("Weather Shield online!");
  }
}

void loop()
{  
  while (client.available() && debug) {   
    char c = client.read();
    if (c) {
      Serial.print(c);
    }
  }
    
  if(millis() - lastConnectionTime > 5000)
  {
    digitalWrite(STAT2, HIGH); //Blink stat LED
    
    calcWeather();
    if (debug) {
      printWeather();
    }
    postWeather();

    digitalWrite(STAT2, LOW); //Turn off stat LED
  }
}

void postWeather() {
  if (client.connected()) {
    client.stop();
  }
  
  if (client.connect(server, 80)) {
    if (debug) {
      Serial.println("Logging sensor data...");
    }
  
    client.print("GET /api/v1.0/Add?bin=3_zE53-m&humidity=");
    client.print(humidity);
    client.print("&temp=");
    client.print(tempf);
    client.print("&pressure=");
    client.print(pressure);
    client.print("&light=");
    client.print(light_lvl);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
    
    if (debug) {
      Serial.println("Log sensors complete");
    }
    
    lastConnectionTime = millis(); 
  } else if (debug) {
    Serial.println("connection failed");
  }
}

void printWeather()
{
  Serial.println();
  Serial.print("humidity=");
  Serial.print(humidity, 1);
  Serial.print(",tempf=");
  Serial.print(tempf, 1);
  Serial.print(",pressure=");
  Serial.print(pressure, 2);
  Serial.print(",light_lvl=");
  Serial.println(light_lvl, 2);
}

void calcWeather()
{
  humidity = myHumidity.readHumidity();
  tempf = myPressure.readTempF();
  pressure = myPressure.readPressure();
  light_lvl = get_light_level();
}

float get_light_level()
{
  float operatingVoltage = analogRead(REFERENCE_3V3);
  float lightSensor = analogRead(LIGHT);
  operatingVoltage = 3.3 / operatingVoltage; //The reference voltage is 3.3V 
  lightSensor = operatingVoltage * lightSensor; 
  return(lightSensor);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
