# Elderly Fall Detection System

## Overview
This project presents an IoT-based Elderly Fall Detection System using ESP8266 and MPU6050 sensor. The system continuously monitors body movements and detects falls using acceleration and orientation changes.

When a fall is detected, an emergency SMS alert is sent to a predefined mobile number using the Twilio SMS API, and a buzzer is activated for local alert.

## Objectives
- Detect falls in elderly people accurately
- Provide immediate emergency alerts
- Reduce response time during accidents
- Ensure reliable remote monitoring

## Hardware Components
- ESP8266 NodeMCU
- MPU6050 (Accelerometer & Gyroscope)
- Buzzer
- Wi-Fi Connection

## Software & Tools
- Arduino IDE
- Embedded C/C++
- Twilio SMS API
- I2C Communication Protocol

## Working Methodology
1. MPU6050 collects acceleration and gyroscope data.
2. Data is processed using a multi-stage trigger-based fall detection algorithm.
3. Sudden free-fall, impact, and orientation change confirm a fall.
4. Buzzer is activated immediately.
5. SMS alert is sent to caregiver via Wi-Fi.

## Review Paper
The detailed review paper explaining fall detection techniques and system design is available in the **review-paper** folder.

## Applications
- Elderly home care
- Hospitals and nursing homes
- Assisted living systems
- Smart healthcare monitoring

## Author
**Aneela**  
B.Tech – Information Technology
