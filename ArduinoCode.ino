#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_ADDR   0x3C // I2C address for OLED display
Adafruit_SSD1306 display(128, 64, &Wire, -1);

#include <OneWire.h>
#include <DallasTemperature.h>

// Temp sensor
#define tempSensor 6   // Digital pin connected to DS18B20 temperature sensor
OneWire oneWire(tempSensor);
DallasTemperature sensors(&oneWire);
float waterTemperature;    // Stores the current water temperature reading

int finaltdsReading;   // Processed TDS reading
int tdsReading;
#define tdsSensorPin A0   // Analog pin for TDS sensor

// Cl2 gas sensor
int cl2Sensor = A1;   // Analog pin for Cl2 gas sensor
float cl2ammount;    // Raw sensor value
int cl2;            // Processed chlorine gas concentration
int Cal_Cl2;        // Calibrated chlorine concentration

// Timer Variables
int minutes;
int seconds;


// Pin Definitions for Switches, Buzzer, and Electrolysis
#define SWT_1 3
#define SWT_2 4
#define SWT_3 5

#define BUZZER_PIN 7  // Buzzer for alarms	
#define electrolysis_PIN 8  // Controls electrolysis process
#define electrolysis_LED_PIN 9   // LED indicator for electrolysis

// Constants
const float F = 96485;    // Faraday's constant (C/mol)
const float M = 74.44;    // Molar mass of NaOCl (g/mol)
const int n = 2;          // Number of electrons for NaOCl formation
const float volume = 0.8; // Volume of brine (liters)
float solution_mass;
float solution_density= 1100; // g/L

// Adjustable system efficiency
float systemEfficiency = 0.744;  // 74.4% efficiency

// Variables
float current = 3.25;          // Current in amperes (A)
float Concentration = 0.000;   // NaOCl concentration in %
float massFormed = 0.00;      // Mass of NaOCl formed (g)
unsigned long timeLeft = 0;   // Time in seconds to achieve required concentration
float targetConcentration = 0.00; // Target concentration based on switch input


unsigned long electrolysisStartTime = 0;  // Start time for electrolysis
bool electrolysisInProgress = false;  // Flag to check if electrolysis is in progress



void setup() {

  Serial.begin(9600);
  sensors.begin();// Initialize temperature sensor

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR); // Initialize OLED display
  display.clearDisplay();// Clear OLED screen


// Configure sensor and actuator pins
  pinMode(cl2Sensor, INPUT);
  pinMode(SWT_1, INPUT);
  pinMode(SWT_2, INPUT);
  pinMode(SWT_3, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(electrolysis_PIN, OUTPUT);
  pinMode(electrolysis_LED_PIN, OUTPUT);


  

  
}

void loop() {


  // Read TDS and temperature
  tdsReading = analogRead(tdsSensorPin);
  finaltdsReading = tdsReading - 100; // Offset calibration
  if (finaltdsReading < 0) { finaltdsReading = 0; }

// Read and process temperature sensor values
  sensors.requestTemperatures();
  waterTemperature = sensors.getTempCByIndex(0);

  // Read and process chlorine gas sensor values
  cl2ammount = analogRead(cl2Sensor);
  cl2 = map(cl2ammount, 0, 4095, 0, 100);   //Convert to percentage
  Cal_Cl2 = cl2 - 90;     // Adjust calibration offset

   if (Cal_Cl2 < 0) { Cal_Cl2 = 0; }
   if (Cal_Cl2 > 5) { buzzer_alarm (); stopElectrolysis(); }  // Stop if Cl2 is too high

  // Switch logic to set target concentration
   if (digitalRead(SWT_1) == HIGH && !electrolysisInProgress) {
    targetConcentration = 0.005;   // 0.5% for disinfection
    startElectrolysis();
  } else if (digitalRead(SWT_2) == HIGH && !electrolysisInProgress) {
    targetConcentration = 0.05;   // 5% for household
    startElectrolysis();
  } else if (digitalRead(SWT_3) == HIGH && !electrolysisInProgress) {
    targetConcentration = 0.1;  // 10% for industrial-grade
    startElectrolysis();
  }
   
  
  if (electrolysisInProgress) {
    manageElectrolysis();
  }

  updateDisplay();

  sendDataToESP();  // Transmit data to ESP32 for remote monitoring
}

void startElectrolysis() {
  electrolysisInProgress = true;
  electrolysisStartTime = millis(); // Record the start time of electrolysis

  // Calculate total time required to reach target concentration considering efficiency
  unsigned long totalTime = (targetConcentration * volume * 10 *n * F * systemEfficiency ) / (current * M);
  timeLeft = totalTime; // Set the initial time left

  digitalWrite(electrolysis_PIN, HIGH);    // Start electrolysis
  digitalWrite(electrolysis_LED_PIN, HIGH); // Turn on LED
}

void manageElectrolysis() {
  unsigned long elapsedTime = (millis() - electrolysisStartTime) / 1000; // Time in seconds

  // Calculate the mass of NaOCl formed based on elapsed time and efficiency
  massFormed = (current * elapsedTime * M * systemEfficiency) / (n * F) ;

  solution_mass = volume * solution_density; 

  // Calculate concentration based on mass formed and volume
  Concentration = ((massFormed / solution_mass)*100); // Convert to percentage

  // Ensure concentration doesn't exceed target
  if (Concentration > targetConcentration) {
    Concentration = targetConcentration;
     stopElectrolysis();
  }

  // Update the time left
  timeLeft = ((targetConcentration * volume * 11 *n * F ) / (current * M * systemEfficiency)) - elapsedTime;

  // Ensure timeLeft does not go negative
  if (timeLeft <= 0) {
    timeLeft = 0;
//    stopElectrolysis();
  }
}

void stopElectrolysis() {
  electrolysisInProgress = false;
  digitalWrite(electrolysis_PIN, LOW);    // Stop electrolysis
  digitalWrite(electrolysis_LED_PIN, LOW); // Turn off LED
  digitalWrite(BUZZER_PIN, HIGH);  // Activate buzzer when target is reached
  delay(1000); // Keep buzzer on for 1 second
  digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer
}

void updateDisplay() {
  display.clearDisplay();

  // Buffer to store formatted strings
  char buffer[10]; 

  // Display NaOCl concentration
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 5);
  display.println("NaOCl:");
  
  
  display.setCursor(35, 5);
  dtostrf(Concentration*100, 6, 4, buffer); // Format Concentration to 3 decimal places
  display.println(buffer);
  display.setCursor(75, 5);
  display.println("%");

  
  display.setCursor(80, 5);
  dtostrf(massFormed, 6, 3, buffer); // Format massFormed to 3 decimal places
  display.println(buffer);
  display.setCursor(120, 5);
  display.println("g");

  // Display time left
  display.setCursor(0, 20);
  display.println("Time Left:");
  display.setCursor(65, 20);
  if (timeLeft > 0) {
//    int minutes = timeLeft / 60;
//    int seconds = timeLeft % 60;

    minutes = timeLeft / 60;
    seconds = timeLeft % 60;
    
    display.print(minutes);
    display.print(":");
    if (seconds < 10) {
      display.print("0");
    }
    display.print(seconds);
  } else {
    display.print("Done");
  }
  

  // Display temperature
  display.setCursor(0, 35);
  display.println("Temp:");
  display.setCursor(40, 35);
  display.println(waterTemperature);
  display.setCursor(85, 35);
  display.println("C");

  // Display Cl2 level
  display.setCursor(0, 50);
  display.println("Cl2:");
  display.setCursor(25, 50);
  display.println(Cal_Cl2);
  display.setCursor(40, 50);
  display.println("%");

  // Display TDS level
  display.setCursor(55, 50);
  display.println("TDS:");
  display.setCursor(80, 50);
  display.println(finaltdsReading);
  display.setCursor(110, 50);
  display.println("ppm");

  // Update OLED display
  display.display();

} 

void buzzer_alarm () {

digitalWrite(BUZZER_PIN, HIGH); 
delay(500); 
digitalWrite(BUZZER_PIN, LOW); 
delay(1000);

}

void sendDataToESP() {
  Serial.print(waterTemperature);
  Serial.print(",");
  Serial.print(finaltdsReading);
  Serial.print(",");
  Serial.print(Concentration*100);
  Serial.print(",");
  Serial.print(cl2);
  Serial.print(",");
  Serial.print(massFormed);
  Serial.print(",");
  Serial.print(minutes);
  Serial.print(",");
  Serial.println(seconds); // End of data line

}
