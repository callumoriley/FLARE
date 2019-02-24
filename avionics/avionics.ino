/*Main Arduino Sketch*/
/*
VERY IMPORTANT PLEASE READ ME! VERY IMPORTANT PLEASE READ ME! VERY IMPORTANT PLEASE READ ME!

                 uuuuuuu
             uu$$$$$$$$$$$uu
          uu$$$$$$$$$$$$$$$$$uu
         u$$$$$$$$$$$$$$$$$$$$$u
        u$$$$$$$$$$$$$$$$$$$$$$$u
       u$$$$$$$$$$$$$$$$$$$$$$$$$u
       u$$$$$$$$$$$$$$$$$$$$$$$$$u
       u$$$$$$"   "$$$"   "$$$$$$u
       "$$$$"      u$u       $$$$"
        $$$u       u$u       u$$$
        $$$u      u$$$u      u$$$
         "$$$$uu$$$   $$$uu$$$$"
          "$$$$$$$"   "$$$$$$$"
            u$$$$$$$u$$$$$$$u
             u$"$"$"$"$"$"$u
  uuu        $$u$ $ $ $ $u$$       uuu
 u$$$$        $$$$$u$u$u$$$       u$$$$
  $$$$$uu      "$$$$$$$$$"     uu$$$$$$
u$$$$$$$$$$$uu    """""    uuuu$$$$$$$$$$
$$$$"""$$$$$$$$$$uuu   uu$$$$$$$$$"""$$$"
 """      ""$$$$$$$$$$$uu ""$"""
           uuuu ""$$$$$$$$$$uuu
  u$$$uuu$$$$$$$$$uu ""$$$$$$$$$$$uuu$$$
  $$$$$$$$$$""""           ""$$$$$$$$$$$"
   "$$$$$"                      ""$$$$""
     $$$"                         $$$$"

In order to successfully poll the GPS, the serial RX buffer size must be increased. This needs
to be done on the computer used for compilation. This can be done by navigating to the following
path in the Arduino contents folder: ‎⁨Contents⁩/⁨Java⁩/⁨hardware⁩/⁨teensy⁩/⁨avr⁩/⁨cores⁩/⁨teensy3⁩/serial1.c.
On line 43 increase SERIAL1_RX_BUFFER_SIZE from 64 to 128.
THIS MUST BE DONE ON THE COMPUTER USED TO COMPILE THE CODE!!!

VERY IMPORTANT PLEASE READ ME! VERY IMPORTANT PLEASE READ ME! VERY IMPORTANT PLEASE READ ME!
*/

/* @file    avionics.ino
 * @author  UBC Rocket Avionics 2018/2019
 * @description The main arduino sketch that controls the flow
 *     of our 30K sensors team code.
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Distributed as-is; no warranty is given.
 */

/*Includes------------------------------------------------------------*/
#include "sensors.h"
#include "statemachine.h"
#include "calculations.h"
#include "commands.h"
#include "gpio.h"
#include "radio.h"

#include <Arduino.h>
#include <HardwareSerial.h>
#include <i2c_t3.h>
#include <SD.h>
#include <string.h>

/*Variables------------------------------------------------------------*/
File radiolog;

/*Functions------------------------------------------------------------*/
/**
  * @brief  The Arduino setup function
  * @param  None
  * @return None
  */
void setup()
{
    initPins();

    bool status = true;

    /*init serial comms*/
    #ifdef TESTING
    SerialUSB.begin(9600);
    while (!SerialUSB) {} //TODO add print in while to see what happens
    SerialUSB.println("Initializing...");
    #endif

    /*init I2C bus*/
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400); //400kHz
    Wire.setDefaultTimeout(100000); //100ms

    /*init sensors*/
    status = initSensors();

    /*if something went wrong spin infinitely, otherwise indicate completion*/
    if (!status) {
        #ifdef TESTING
        SerialUSB.println("Initialization failed! >:-{");
        #else
        while (1) {}
        #endif
    } else {
        pinMode(LED_BUILTIN,OUTPUT);
        digitalWrite(LED_BUILTIN,HIGH);
        #ifdef TESTING
        SerialUSB.println("Initialization complete! :D");
        #endif
    }
}

/**
  * @brief  The Arduino loop function
  * @param  None
  * @return None
  */
void loop()
{
    static unsigned long timestamp;
    static float barometer_data_init = barSensorInit();
    static float baseline_pressure = groundAlt_init(&barometer_data_init);  // IF YOU CAN'T DO THIS USE GLOBAL VAR
    static unsigned long old_time = 0; //ms
    static unsigned long new_time = 0; //ms

    static unsigned long tier_one_old_time = 0;
    static unsigned long tier_two_old_time = 0;
    static unsigned long tier_three_old_time = 0;

    unsigned long delta_time;

    static uint16_t time_interval = 50; //ms

    static float battery_voltage, acc_data[ACC_DATA_ARRAY_SIZE], bar_data[BAR_DATA_ARRAY_SIZE],
        temp_sensor_data, IMU_data[IMU_DATA_ARRAY_SIZE], GPS_data[GPS_DATA_ARRAY_SIZE];
    static float prev_altitude, altitude, delta_altitude, prev_delta_altitude, ground_altitude;
    static FlightStates state = ARMED;

    static uint16_t tier_one_interval = 400;
    static uint16_t tier_two_interval = 2000;
    static uint16_t tier_three_interval = 20000;

    char command[RADIO_DATA_ARRAY_SIZE];
    char recognitionRadio[RADIO_DATA_ARRAY_SIZE];
    char goodResponse[] = {'G','x','x','x','x'};
    const char badResponse[] = {'B','B','B','B','B'};

    if (SerialRadio.available() == 5) {

        #ifdef TESTING
        SerialUSB.print("Received Message: ");
        #endif

        for(int i = 0; i< RADIO_DATA_ARRAY_SIZE; i++){
            command[i] = SerialRadio.read();
        }


        bool correctCommand = check(command);

        if(correctCommand){
            for(int i =1; i<5; i++)
            {
                goodResponse[i] = command[0];
            }

            #ifdef TESTING
            SerialUSB.print("Good command: ");
            SerialUSB.println(command);
            #endif

            doCommand(command[0], &state);
            sendRadioResponse(goodResponse);
        }
        else{
            #ifdef TESTING
            SerialUSB.print("Bad command: ");
            SerialUSB.println(command);
            #endif

            sendRadioResponse(badResponse);
        }
    }

    new_time = millis();
    if ((new_time - old_time) >= time_interval) {
        delta_time = new_time - old_time;
        old_time = new_time;
        pollSensors(&timestamp, &battery_voltage, acc_data, bar_data, &temp_sensor_data, IMU_data, GPS_data);
        calculateValues(acc_data, bar_data, &prev_altitude, &altitude, &delta_altitude, &prev_delta_altitude, &baseline_pressure, &delta_time);
        stateMachine(&altitude, &delta_altitude, &prev_altitude, bar_data, &baseline_pressure, &ground_altitude, &state);
        logData(&timestamp, &battery_voltage, acc_data, bar_data, &temp_sensor_data, IMU_data, GPS_data, state, altitude, baseline_pressure);
    }

    if((new_time - tier_one_old_time) >= tier_one_interval) {
        sendTierOne(&timestamp, GPS_data, bar_data, state, altitude);
        tier_one_old_time = new_time;
    }

    if ( (new_time - tier_two_old_time) >= tier_two_interval ){
        sendTierTwo(acc_data, bar_data, &temp_sensor_data, IMU_data);
        tier_two_old_time = new_time;
    }

     if ( (new_time - tier_three_old_time) >= tier_three_interval ){
        sendTierThree(&battery_voltage, &ground_altitude);
        tier_three_old_time = new_time;
    }





    #ifdef TESTING
    delay(1000);
    #endif
}

//checks if all indexes are equal for radio commands
bool check(char *radioCommand)
 {
    const char a0 = radioCommand[0];

    for (int i = 1; i < 5; i++)
    {
        if (radioCommand[i] != a0)
            return false;
    }
    return true;
}
