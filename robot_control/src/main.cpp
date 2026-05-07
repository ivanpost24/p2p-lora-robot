
// Use for testing joy stick

#include <Arduino.h>
#include <Wire.h>
#include <HCSR04.h>
#include <Adafruit_BNO055.h>

//#include "config.h"
//#include "display.h"
//#include "joystick.h"

UltraSonicDistanceSensor distanceSensor(7, 6);
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);

void displaySensorDetails(void)
{
  sensor_t sensor;
  bno.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" xxx");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" xxx");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" xxx");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Beginning display init");

    Wire.begin(35, 36);
    if(!bno.begin()) {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
    }
    displaySensorDetails();
    bno.setExtCrystalUse(true);
}

void loop() {
    float distance = distanceSensor.measureDistanceCm();
    Serial.printf("Your hand is\t%0.02f\t cm away\n", distance);
    /* Get a new sensor event */
    sensors_event_t event;
    bno.getEvent(&event);

    /* Display the floating point data */
    Serial.print("X: ");
    Serial.print(event.orientation.x, 4);
    Serial.print("\tY: ");
    Serial.print(event.orientation.y, 4);
    Serial.print("\tZ: ");
    Serial.print(event.orientation.z, 4);
    Serial.println("\n");
    delay(100);
}