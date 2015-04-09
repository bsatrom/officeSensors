#include <Wire.h> //I2C needed for sensors
#include "MPL3115A2.h" //Pressure sensor
#include "HTU21D.h" //Humidity sensor

// Includes for WiFi Connectivity
#include <SPI.h>
#include <SFE_CC3000.h>
#include <SFE_CC3000_Client.h>

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

//Hardware pins for CC3000 WiFi chip
#define CC3000_INT      2   // Needs to be an interrupt pin (D2/D3)
#define CC3000_EN       7   // Can be any digital pin
#define CC3000_CS       10  // Preferred is pin 10 on Uno

// Connection info data lengths
#define IP_ADDR_LEN     4   // Length of IP address in bytes

// Constants
#define ap_security WLAN_SEC_WPA2
#define timeout 30000

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
SFE_CC3000 wifi = SFE_CC3000(CC3000_INT, CC3000_EN, CC3000_CS);
SFE_CC3000_Client client = SFE_CC3000_Client(wifi);

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
  ConnectionInfo connection_info;
  int i;
  
  if (debug) {
    Serial.begin(115200);
    Serial.println();
    Serial.println("Connecting to WiFi...");
  }
  
  wifi.init();
  
  // Connect to WiFi network stored in non-volatile memory
  if ( !wifi.fastConnect(timeout) && debug ) {
    Serial.println("Error: Could not connect to network");
  }
  
  // Gather connection details and print IP address
  if (debug) {
    if ( !wifi.getConnectionInfo(connection_info) ) {
      Serial.println("Error: Could not obtain connection details");
    } else {
      Serial.print("Connected to: ");
      Serial.println(connection_info.ssid);
      Serial.print("IP Address: ");
      for (i = 0; i < IP_ADDR_LEN; i++) {
        Serial.print(connection_info.ip_address[i]);
        if ( i < IP_ADDR_LEN - 1 ) {
          Serial.print(".");
        }
      }
      Serial.println();
    }
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
  //Calc humidity
  humidity = myHumidity.readHumidity();
  
  //Calc tempf from pressure sensor
  tempf = myPressure.readTempF();
  
  //Calc pressure
  pressure = myPressure.readPressure();
 
  //Calc light level
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
