#define BLYNK_TEMPLATE_ID "TMPL27qxa-esg"
#define BLYNK_TEMPLATE_NAME "Bleach Manufacture"
#define BLYNK_AUTH_TOKEN "LYJwR_AsZX44ByF_VbWnpRJ7p38nDRPu"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "ion";  // Enter your wifi name
char pass[] = "12345678";  // Enter your wifi password

float waterTemperature, finaltdsReading, Concentration, cl2, massFormed;
int minutes, seconds;

int wifiFlag = 0;

char auth[] = BLYNK_AUTH_TOKEN;
BlynkTimer timer;

void checkBlynkStatus() { // called every 2 seconds by SimpleTimer
bool isconnected = Blynk.connected();
if (isconnected == false) {
wifiFlag = 1;
// Serial.println("Blynk Not Connected");
//digitalWrite(wifiLed, LOW);
}
if (isconnected == true) {
wifiFlag = 0;
//digitalWrite(wifiLed, HIGH);
//Serial.println("Blynk Connected");
}
}

BLYNK_CONNECTED() {
// update the latest state to the server

Blynk.syncVirtual(V0);
Blynk.syncVirtual(V1);
Blynk.syncVirtual(V2);
Blynk.syncVirtual(V3);
Blynk.syncVirtual(V4);
Blynk.syncVirtual(V5);
Blynk.syncVirtual(V6);

}


void sendSensor()
{
  
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
    
    
    
  Blynk.virtualWrite(V0, waterTemperature);
  Blynk.virtualWrite(V1, finaltdsReading);
  Blynk.virtualWrite(V2, Concentration *100);
  Blynk.virtualWrite(V3, cl2);

  Blynk.virtualWrite(V4, massFormed);
  Blynk.virtualWrite(V5, minutes);
  Blynk.virtualWrite(V6, seconds);

   
    
}

void receiveSerialData() {
  if (Serial.available()) {  // Read from Arduino Nano
    String data = Serial.readStringUntil('\n'); // Read until newline
    sscanf(data.c_str(), "%f,%f,%f,%f,%f,%d,%d", &waterTemperature, &finaltdsReading, &Concentration, &cl2, &massFormed, &minutes, &seconds);
    Serial.println("Received Data: " + data);
  }
}

void setup() {

  Serial.begin(9600);
 

  WiFi.begin(ssid, pass);
timer.setInterval(2000L, checkBlynkStatus); 
// check if Blynk server is connected every 2seconds
timer.setInterval(100L, sendSensor);
// Sending Sensor Data to Blynk Cloud every 1 second
Blynk.config(auth);

  
}

void loop() {

Blynk.run();
timer.run();

}

