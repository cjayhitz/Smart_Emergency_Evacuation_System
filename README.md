# Smart Emergency Evacuation System
## Contributors:
## 1. Veytri
## 2. Naqib

This project implements a Smart Emergency Evacuation System using an ESP32 microcontroller, sensors, and actuators. 
The system monitors environmental conditions using a DHT11 temperature/humidity sensor and an MQ2 gas sensor. 
It publishes telemetry data to the V-One IoT Cloud for real-time monitoring and triggers emergency actions when thresholds are exceeded.

### Features

#### Real-Time Telemetry:
Monitors temperature, humidity, and gas levels.
Publishes sensor data to the V-One IoT Cloud via MQTT.

#### Automated Emergency Actions:
Activates a fan (relay) and opens a door (servo motor) when gas levels exceed a threshold.
Sequentially blinks LEDs to signal an emergency.

#### Manual Override:
Allows manual system deactivation using a push button.

#### Cloud Integration:
Uses V-One IoT Cloud for real-time monitoring, actuator control, and notifications.
