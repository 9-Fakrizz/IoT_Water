#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 16 // Pin to connect temperature sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* lineNotifyToken = "YourLineNotifyToken";

#define VREF 1 // Analog reference voltage(Volt) of the ADC
#define anal_ph_pin 32
#define anal_tds_pin 35
#define anal_turb_pin 34

unsigned long previousMillis = 0;
const long interval = 10000; // 10 seconds

long ph_value;
float phAvg;
float tdsValue, tdsAvg;
float turb;
float turb_avg = 0;

float pH_send;
float temp_send;
float tds_send;
float turb_send;

// Function declarations
void sendLineNotification(const char* message);
void Read_ph_temp();
void Read_tds();
void Read_turb();

void setup() {
  Serial.begin(115200);
  sensors.begin();
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  // Send Line notification
  sendLineNotification("Hello from ESP32!");
}

void loop() {
  unsigned long currentMillis = millis();
  Read_Every_Analog();

  // Check if 10 seconds have passed
  if (currentMillis - previousMillis >= interval) {
    // Save the current time
    previousMillis = currentMillis;

    // Read all sensor values
    Read_ph_temp();
    Read_turb();
    Read_tds();

    // Prepare a message with all sensor values
    String message = "\npH: " + String(pH_send) + "\n" +
                     "Temperature: " + String(temp_send) + " *C" + "\n" +
                     "TDS: " + String(tds_send) + " ppm" + "\n" +
                     "Turbidity: " + String(turb_send);

    // Send Line Notification with the message
    sendLineNotification(message.c_str());
  }
}

// Function to send Line notification
void sendLineNotification(const char* message) {
  HTTPClient http;

  // Line Notify API endpoint
  String url = "https://notify-api.line.me/api/notify";

  // Set up the HTTP request headers
  http.begin(url.c_str());
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", "Bearer " + String(lineNotifyToken));

  // Prepare the data to be sent
  String postData = "message=" + String(message);

  // Send the HTTP POST request
  int httpResponseCode = http.POST(postData);

  // Check for errors
  if (httpResponseCode > 0) {
    Serial.print("Line Notification sent successfully. Response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error sending Line Notification. HTTP Response code: ");
    Serial.println(httpResponseCode);
  }

  // Close the connection
  http.end();
}

// Function to read TDS sensor values
void Read_tds() {
  float temperature = 25;
  tdsValue = 0;
  tdsAvg = 0;
  float tdsVoltage = 0;

  for (int i = 0; i < 30; i++) {
    tdsValue += analogRead(anal_tds_pin);
    delay(10);
  }
  tdsAvg = tdsValue / 30;
  tdsAvg = map(tdsAvg, 0, 4096, 0, 1023);

  tdsVoltage = tdsAvg * (3.0 / 1023.0); // Convert sensor reading into millivolt

  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVolatge = tdsVoltage / compensationCoefficient;
  tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5;

  tds_send = tdsValue;
  Serial.print("TDS Value:");
  Serial.print(tdsValue, 0);
  Serial.println(" ppm");
}

// Function to read pH and temperature sensor values
void Read_ph_temp() {
  ph_value = 0;
  phAvg = 0;
  float C = 23.85;
  float m = -6.80;

  for (int i = 0; i < 10; i++) {
    ph_value += analogRead(anal_ph_pin);
    delay(10);
  }

  phAvg = ph_value / 10;
  float phVoltage = phAvg * (3.3 / 4096.0);
  float pHValue = phVoltage * m + C;
  pH_send = pHValue;
  temp_send = sensors.getTempCByIndex(0);

  Serial.print("phVoltage = ");
  Serial.print(phVoltage);
  Serial.print(" ");
  Serial.print("pH=");
  Serial.println(pHValue);

  sensors.requestTemperatures();
  Serial.print("Temperature is: ");
  Serial.print(sensors.getTempCByIndex(0));
  Serial.println(" *C");
  Serial.println("");
}

// Read Turbidity
void Read_turb(){
  turb_avg = 0;
  for(int i = 0; i < 500 ; i++){
    turb = analogRead(34);
    turb_avg += turb;
  }
  turb_avg = turb_avg / 500;
  turb_avg = map(turb_avg, 0, 15, 0, 10);
  Serial.println("Turbdt : " + String(turb_avg)); 
  turb_send = turb; 
}
