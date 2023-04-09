/*
This code was developed as part of the "Development of a Laboratory Bench for Comparison Measurement of Relative Air Humidity" project,
which was a thesis for the University of Rousse "Angel Kanchev." The project was coded and developed by Kaloyan Dimitrov.

The objective of this project was to design and build a laboratory bench that can accurately and reliably measure relative air humidity,
and facilitate the comparison of different measurement devices and methods. The bench has practical applications in industries such as meteorology,
agriculture, and HVAC, and represents an important contribution to the field of air quality measurement.

For any questions or comments regarding the project, please refer to the thesis documentation.
*/


#include <Adafruit_Sensor.h> // Load sensor library
#include <Adafruit_AHTX0.h> // Load ATH11 library
#include <Wire.h> // Load Wire library
#include <OneWire.h> // Load One Wire library
#include <LiquidCrystal_I2C.h> // Load LCD I2C library
#include "DHT.h" // Load DHT11 library
#include <WiFi.h>// Load Wi-Fi library
#include <WebServer.h>// Load Web server library
#include <DallasTemperature.h> //Load Dallas Temperature library
#include <cmath>

// Replace with your network credentials Wifi credentials 
const char* ssid = "SSID";
const char* password = "PASS";

// Define the pins for the buttons
#define BUTTON_1_PIN 33 // ESP32 pin GIOP32 connected to button 1
#define BUTTON_2_PIN 32 // ESP32 pin GIOP33 connected to button 2

#define DS18B20Pin 26 // ESP32 pin GIOP33 connected to DS18B20 sensors
OneWire DS18B20OneWire(DS18B20Pin);// Setup a oneWire instance to communicate with any OneWire device
DallasTemperature DS18ReadOut(&DS18B20OneWire);// Pass oneWire reference to DallasTemperature library

//Define sensors addresses over One Wire
DeviceAddress DrySensor1 = { 0x28, 0x90, 0xBD, 0x48, 0xF6, 0x3E, 0x3C, 0x40 };
DeviceAddress WetSensor1 = { 0x28, 0x8F, 0x76, 0x49, 0xF6, 0x37, 0x3C, 0x50 };

#define DHTPIN  27 // ESP32 pin GIOP27 connected to DHT11 sensor

#define DHTTYPE DHT11// DHT11 define type

DHT dht_sensor(DHTPIN, DHTTYPE);//Initialize DHT11

// Define the menu options
#define MENU_OPTIONS 4 //Total menu options
String menu[MENU_OPTIONS] = {"DS18B20", "DHT11","AHT10","IP"}; //Menu options array

#define AHT10_I2C_ADDR 0x38 // Define the I2C addresses of the AHT10 sensor
#define LCD_I2C_ADDR 0x27 // Define the I2C addresses of the LCD

// Set the dimensions of the LCD (20 columns and 4 rows)
#define LCD_COLS 20
#define LCD_ROWS 4

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);// Define the LCD screen

// Declare temperature variables in Celsius - DS18B20
float DrySensor = 0.0;//DrySensor1
float WetSensor = 0.0;//WetSensor1
float humidityDS18B20 = 0.0; //To Do Formula
double atmosphericPressure = 1.01e5; // in Pa 
float DHTHumidity  = 0.0; // Declare humidity variable - DHT11
float DHTTemperatureC = 0.0; //  Declare temperature variables in Celsius - DHT11

float AHTTemperatureC = 0.0; // Declare humidity variable - AH10
float AHTHumidity  = 0.0; //  Declare temperature variables in Celsius - AH10
sensors_event_t humidity, temp; //  Set event for humidity and temp
Adafruit_AHTX0 aht10;// Create an instance of the AHT10 sensor

int currentOption = 0;//Declare Current menu option variable

WebServer server(80);// Create WebServer object on port 80

void setup() {

  // Initialize the Serial communication
  Serial.begin(9600);



  // Initialize the LCD screen
  lcd.init();
  lcd.backlight();

  dht_sensor.begin();// Initialize the DHT11 sensor
  
  Wire.begin();// Start the Wire communication
  
//Display prompt
  lcd.setCursor(0, 0);
  lcd.print("RELATIVE HUMIDITY");
  lcd.setCursor(0, 1);
  lcd.print("UNI RUSE");
  lcd.setCursor(0, 2);
  lcd.print("\"ANAGEL KANCHEV\"");
  delay(3000);
  lcd.clear();


 // Initialize the AHT10 sensor
  if (!aht10.begin(&Wire,AHT10_I2C_ADDR)) {
    lcd.setCursor(0, 0);
    lcd.print("Could not find");
    lcd.setCursor(0, 1);
    lcd.print("AHT10 sensor!");
    Serial.println("Could not find AHT10 sensor!");
    lcd.clear();
    //while (1);
  }
  // Set up the buttons
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);



 // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  if (WiFi.status() != WL_CONNECTED) { 
    Serial.println("Connected to WiFi");
    lcd.setCursor(0, 0);
    lcd.print("Connected to ");
    lcd.print(ssid);
    delay(2000);
    lcd.clear();
  }else{
    Serial.println("Connecting to WiFi..");
    lcd.setCursor(0, 0);
    lcd.print("Connecting to WiFi..");
    delay(2000);
    lcd.clear(); 
  }
  

//Web server Initialize
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();

 
 
}

unsigned long previousMillis = 0;
const long interval = 50;

void loop() {

  // Handle the input button presses
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
  DS18ReadOut.requestTemperatures(); 
  aht10.getEvent(&humidity, &temp);
  AHTTemperatureC=temp.temperature;// read AHT10 temperature
  AHTHumidity=humidity.relative_humidity;// read ATH10 relative_humidity
  DrySensor = DS18ReadOut.getTempC(DrySensor1); // read DS18B20 Dry Sensor
  WetSensor =  DS18ReadOut.getTempC(WetSensor1); // read DS18B20 Wet Sensor
  humidityDS18B20=calculateRelativeHumidity(DrySensor, WetSensor, atmosphericPressure);
  DHTHumidity  = dht_sensor.readHumidity();// read DHT11 humidity
  DHTTemperatureC = dht_sensor.readTemperature();// read DHT11 temperature in Celsius

    previousMillis = currentMillis;
    if (digitalRead(BUTTON_1_PIN) == LOW) {
      currentOption = (currentOption + 1) % MENU_OPTIONS;
      lcd.clear();
    }

    if (digitalRead(BUTTON_2_PIN) == LOW) {
      currentOption = (currentOption - 1 + MENU_OPTIONS) % MENU_OPTIONS;
      lcd.clear();
    }
  }

  // Display the selected sensor value on the LCD screen
  if (currentOption == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Dry Sensor:");
    lcd.print(DrySensor, 1);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("Wet Sensor:");
    lcd.print(WetSensor, 1);
    lcd.print(" C");

    lcd.setCursor(0, 2);
    lcd.print("Relative humidity:");
    lcd.setCursor(0, 3);    
    lcd.print(humidityDS18B20, 1);
    lcd.print(" %");
  }
  else if (currentOption == 1) {
    lcd.setCursor(0, 0);
    lcd.print(menu[currentOption]);

    lcd.setCursor(0, 1);
    lcd.print("Temperature: ");
    lcd.print(DHTTemperatureC, 1);
    lcd.print(" C");

    lcd.setCursor(0, 2);
    lcd.print("Humidity: ");
    lcd.print(DHTHumidity, 1);
    lcd.print(" %");
  }
  else if (currentOption == 2) {
    lcd.setCursor(0, 0);
    lcd.print(menu[currentOption]);

    lcd.setCursor(0, 1);
    lcd.print("Temperature: ");
    lcd.print(AHTTemperatureC, 1);
    lcd.print(" C");

    lcd.setCursor(0, 2);
    lcd.print("Humidity: ");
    lcd.print(AHTHumidity, 1);
    lcd.print(" %");
  }
  else if (currentOption == 3) {
    lcd.setCursor(0, 0);
    lcd.print(menu[currentOption]);
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
  }

  server.handleClient();
}

// Handle Http 200 request
void handle_OnConnect() {
  server.send(200, "text/html", SendHTML(DHTTemperatureC,DHTHumidity,DrySensor,WetSensor,humidityDS18B20,AHTTemperatureC,AHTHumidity)); 
}
// Handle Http 404 request
void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}
// HTML Payload
String SendHTML(float DHTTemperatureCStat,float DHTHumidityStat,float DrySensorStat,float WetSensorStat,float humidityDS18B20Stat,float AHTTemperatureC,float AHTHumidity){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta charset=utf-8 name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>ESP32 Weather Report</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div style=\"display:flex;flex-wrap:wrap;justify-content: space-evenly;align-items: center;border-bottem:2px solid black\"  id=\"logos<\">\n";
    ptr +="<div style=\"\">\n";
    ptr +="<img src=\"https://www.uni-ruse.bg/_layouts/15/images/UniRuse/ru-default-logo.png\" alt=\"Uni-Ruse-logo\" height=\"150px\" width=\"150px\">";
    ptr +="<h1>Русенски университет \"Ангел Кънчев\"</h1>";
    ptr +="</div>";
    ptr +="<div style=\"\">\n";
    ptr +="<img src=\"https://www.uni-ruse.bg/Faculties/FEEA/PublishingImages/logo_EEA_BG.jpg\" alt=\"Uni-Ruse-logo\" height=\"150px\" width=\"150px\">";
    ptr +="<h1>Факултет Електротехника, електроника и автоматика</h1>";
    ptr +="</div>";
  ptr +="</div>";
  ptr +="<div style=\"display:flex;flex-wrap:wrap;justify-content: space-evenly;\"  id=\"sensors<\">\n";
  ptr +="<div id=\"lm35<\">\n";
  ptr +="<h1>DS18B20: Dry Sensor</h1>\n";
  ptr +="<p>Temperature: ";
  ptr +=(float)DrySensorStat;
  ptr +=" C</p>";
  ptr +="<h1>DS18B20: Wet Sensor</h1>\n";
  ptr +="<p>Temperature: ";
  ptr +=(float)WetSensorStat;
  ptr +=" C</p>";
  ptr +="<h1>DS18B20: Relative humidity</h1>\n";
  ptr +="<p>Humidity: ";
  ptr +=(float)humidityDS18B20Stat;
  ptr +=" %</p>";
  ptr +="</div>\n";
  ptr +="<div id=\"dht11\">\n";
  ptr +="<h1>DHT 11</h1>\n";
  ptr +="<p>Temperature: ";
  ptr +=(float)DHTTemperatureCStat;
  ptr +=" C</p>";
  ptr +="<p>Humidity: ";
  ptr +=(float)DHTHumidityStat;
  ptr +=" %</p>";
  ptr +="</div>\n";
  ptr +="<div id=\"aht10\">\n";
  ptr +="<h1>AHT 10</h1>\n";
  ptr +="<p>Temperature: ";
  ptr +=(float)AHTTemperatureC;
  ptr +=" C</p>";
  ptr +="<p>Humidity: ";
  ptr +=(float)AHTHumidity;
  ptr +=" %</p>";
  ptr +="</div>\n";
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  ptr +="<script>setInterval(function() {location.reload();}, 2000);</script>";
  return ptr;
}
double calculateRelativeHumidity(double DrySensor, double WetSensor, double atmosphericPressure) {
    float Pm= 479+pow(11.52+1.62*WetSensor,2);
    float Ps= 479+pow(11.52+1.62*DrySensor,2);
    float A=60e-5;
    double relativeHumidity=((Pm-atmosphericPressure*A*(DrySensor-WetSensor))/Ps)*100;
    return relativeHumidity;
}
