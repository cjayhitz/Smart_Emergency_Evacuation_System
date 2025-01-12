#include "VOneMqttClient.h"
#include "DHT.h"
#include <ESP32Servo.h> // Using ESP32Servo library for ESP32 compatibility

// Define device IDs
const char* DHT11sensor = "72751cc7-794a-4683-8d02-a30b6383b96e";
const char* MQ2sensor = "0f3aae92-b6d9-4f04-ae6c-4fa2d9cc5495";
const char* RelayDevice = "dc00a441-fcb6-41e6-9111-51f54e0a85f2";
const char* ServoMotorDevice = "86bd1720-a350-463e-8c29-62eb00153585";

// Used Pins
const int dht11Pin = 42;
const int MQ2pin = A5;
const int ledPins[] = {38, 39, 40};
const int relayPin = 14;
const int servoPin = 48;
const int buttonPin = 21; // Define the push button pin

// Input sensor
#define DHTTYPE DHT11
DHT dht(dht11Pin, DHTTYPE);

// Create an instance of VOneMqttClient
VOneMqttClient voneClient;
Servo myServo;

// Last message time
unsigned long lastMsgTime = 0;
const unsigned long TELEMETRY_INTERVAL = 5000; // 5 seconds interval for telemetry

// Thresholds
const int gasThreshold = 800;

// Flags for system state
bool ledBlinking = false;
bool fanRunning = false; // Flag to track fan state
bool doorOpened = false; // Flag to track door state

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  voneClient.setup();
  voneClient.registerActuatorCallback(triggerActuator_callback); // Register the callback

  dht.begin();
  Serial.println("Gas sensor warming up!");
  delay(20000);

  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  myServo.attach(servoPin);
  myServo.write(0);

  pinMode(buttonPin, INPUT_PULLUP); // Set up the push button pin
}

void loop() {
  if (!voneClient.connected()) {
    voneClient.reconnect();
    voneClient.publishDeviceStatusEvent(DHT11sensor, false, "DHT11 Sensor Disconnected");
    voneClient.publishDeviceStatusEvent(MQ2sensor, false, "MQ2 Sensor Disconnected");
    voneClient.publishDeviceStatusEvent(RelayDevice, false, "Relay Disconnected");
    voneClient.publishDeviceStatusEvent(ServoMotorDevice, false, "Servo Motor Disconnected");
  }
  voneClient.loop();

  unsigned long cur = millis();
  if (cur - lastMsgTime > TELEMETRY_INTERVAL) {
    lastMsgTime = cur;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      voneClient.publishDeviceStatusEvent(DHT11sensor, false, "DHT11 Sensor Read Error");
    } else {
      JSONVar dhtPayload;
      dhtPayload["Humidity"] = h;
      dhtPayload["Temperature"] = t;
      voneClient.publishTelemetryData(DHT11sensor, dhtPayload);
    }

    float gasValue = analogRead(MQ2pin);
    if (isnan(gasValue)) {
      Serial.println("Failed to read from MQ2 sensor!");
      voneClient.publishDeviceStatusEvent(MQ2sensor, false, "MQ2 Sensor Read Error");
    } else {
      Serial.print("Gas Value: ");
      Serial.println(gasValue);
      voneClient.publishTelemetryData(MQ2sensor, "Gas detector", gasValue);

      if (gasValue > gasThreshold) {
        if (!ledBlinking) {
          ledBlinking = true;
        }
        if (!fanRunning) {
          digitalWrite(relayPin, HIGH);
          fanRunning = true;
          JSONVar relayState;
          relayState["Relay"] = true;
          voneClient.publishActuatorStatusEvent(RelayDevice, relayState, true);
        }
        if (!doorOpened) {
          myServo.write(90);
          doorOpened = true;
          JSONVar servoState;
          servoState["Door"] = "Opened";
          voneClient.publishActuatorStatusEvent(ServoMotorDevice, servoState, true);
        }
      }
    }

    if (ledBlinking) {
      blinkLEDsSequentially();
    }
  }

  // Manual override using push button
  if (digitalRead(buttonPin) == LOW) {
    Serial.println("Manual button pressed. Turning off systems.");

    // Turn off relay
    digitalWrite(relayPin, LOW);
    fanRunning = false;

    // Turn off LEDs
    for (int i = 0; i < 3; i++) {
      digitalWrite(ledPins[i], LOW);
    }
    ledBlinking = false;

    // Reset servo position
    myServo.write(0);
    doorOpened = false;
  }
}

void triggerActuator_callback(const char* actuatorDeviceId, const char* actuatorCommand) {
  Serial.print("Received actuator command for: ");
  Serial.print(actuatorDeviceId);
  Serial.print(" with command: ");
  Serial.println(actuatorCommand);

  JSONVar commandObject = JSON.parse(actuatorCommand);
  JSONVar keys = commandObject.keys();

  if (String(actuatorDeviceId) == RelayDevice) {
    bool commandValue = commandObject[keys[0]];
    digitalWrite(relayPin, commandValue);
    Serial.println(commandValue ? "Relay ON" : "Relay OFF");
    
    JSONVar relayState;
    relayState["command"] = commandValue ? "ON" : "OFF";
    voneClient.publishActuatorStatusEvent(actuatorDeviceId, relayState, true);
  }

  if (String(actuatorDeviceId) == ServoMotorDevice) {
    int angle = commandObject[keys[0]];
    myServo.write(angle);
    Serial.print("Servo set to angle: ");
    Serial.println(angle);
    
    JSONVar servoState;
    servoState["command"] = angle;
    voneClient.publishActuatorStatusEvent(actuatorDeviceId, servoState, true);
  }
}

void blinkLEDsSequentially() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], HIGH);
    delay(500);
    digitalWrite(ledPins[i], LOW);
    delay(500);
  }
}