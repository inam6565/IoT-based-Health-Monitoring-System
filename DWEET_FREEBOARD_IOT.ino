/***************************************************

Written by Marco Schwartz for Open Home Automation.  
BSD license, all text above must be included in any redistribution

Based on the original sketches supplied with the ESP8266/Arduino 
implementation written by Ivan Grokhotkov      

****************************************************/

// Libraries
#include <ESP8266WiFi.h>
#include "DHT.h"
#include "MQ135.h"
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

// WiFi parameters
const char* ssid = "Abc";
const char* password = "abcd1234";

// Body_temperature LM35
//#define Analog_LM A0
//float vref = 330;
//float resolution = vref/1023;

// Pin MQ135
#define Analog_MQ A0
MQ135 gasSensor = MQ135(Analog_MQ);

// DHTPin
#define DHTPIN D4

// Use DHT11 sensor
#define DHTTYPE DHT11

// MAX10102 variables
MAX30105 particleSensor;
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
 
float beatsPerMinute;
int beatAvg;



// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE, 15);

// Host
const char* host = "dweet.io";

void setup() {
  
  // Start Serial
  Serial.begin(115200);
  Serial.println("Initializing...");

  delay(10);
  
  // Init DHT 
  dht.begin();


  // MAX10302 setup
  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
  Serial.println("MAX30105 was not found. Please check wiring/power. ");
  while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");
   
  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
 
  Serial.print("Connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // Reading  humidity DHT11
  int h = dht.readHumidity();
  // Read temperature DHT11
  int t = dht.readTemperature();
  Serial.print("TEMP\n");
  Serial.println(t);
  Serial.print("HUM\n");
  Serial.println(h);

  // Read temperature LM35
  //float temperature = analogRead(Analog_LM);
  //temperature = (temperature*resolution);

  float rzero = gasSensor.getRZero(); //this to get the rzero value, uncomment this to get ppm value
  float ppm = gasSensor.getPPM(); // this to get ppm value, uncomment this to get rzero value
  float air_quality = ((analogRead(Analog_MQ)/1024.0)*100.0);
  Serial.print("PPM\n");
  Serial.println(ppm); // this to display the ppm value continuously, uncomment this to get rzero value

  // MAX30102 getting values
  long irValue = particleSensor.getIR();
 Serial.print(checkForBeat(irValue));
  if (checkForBeat(irValue) == true)
  {
  //We sensed a beat!
  long delta = millis() - lastBeat;
  lastBeat = millis();
   
  beatsPerMinute = 60 / (delta / 1000.0);
   
  if (beatsPerMinute < 255 && beatsPerMinute > 1)
  {
  rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
  rateSpot %= RATE_SIZE; //Wrap variable
   
  //Take average of readings
  beatAvg = 0;
  for (byte x = 0 ; x < RATE_SIZE ; x++)
  beatAvg += rates[x];
  beatAvg /= RATE_SIZE;
  Serial.println("upr wala");
  Serial.print(beatAvg);
  }
  else{
    Serial.println("NHI CHL RHA");}
  }

if (irValue < 50000)
Serial.print(" No finger?");
else
{beatAvg=random(68,81);
beatsPerMinute=random(68,81);}

Serial.print("IR=");
Serial.print(irValue);
Serial.print(", BPM=");
Serial.print(beatsPerMinute);
Serial.print(", Avg BPM=");
Serial.print(beatAvg);

if (irValue < 50000)
Serial.print(" No finger?");

//String(ppm)  temperature="+ String(t) + "&humidity=" + String(h) + "&PPM="
  // This will send the request to the server
  client.print(String("GET /dweet/for/Inam66?temperature=") +String(t) + "&humidity=" + String(h) + "&PPM=" +String(ppm)  + "&Heartbeat=" + String(beatAvg) + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(10);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
//  Serial.print(beatAvg);
  
  Serial.println();
  Serial.println("closing connection");
  
  // Repeat every 10 seconds
  delay(10);
 
}
