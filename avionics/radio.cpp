#include "radio.h"

#include "sensors.h"
#include "battery.h"
#include "SparkFun_LIS331.h"        //accelerometer
#include "MS5803_01.h"              //barometer
#include "SparkFunTMP102.h"         //temp sensor
#include "Adafruit_BNO055.h"        //IMU
#include "Venus638FLPx.h"           //GPS

#include <Arduino.h>
#include <HardwareSerial.h>
#include <i2c_t3.h>
#include <SD.h>



void processRadioData(unsigned long *timestamp, float* battery_voltage, float acc_data[], float bar_data[],
    float *temp_sensor_data, float IMU_data[], float* GPS_data, FlightStates state, float altitude){

}
//send pressure, gps, state, and altitude over radio
void sendTierOne(unsigned long *timestamp, float* GPS_data, float bar_data[], FlightStates state, float altitude){
    float time = *timestamp;
    sendRadioData(time, 't');
    sendRadioData(bar_data[0], UID_bar_pres);
    sendRadioData(GPS_data[0], UID_GPS_lat);
    sendRadioData(GPS_data[1], UID_GPS_long);
    sendRadioData(GPS_data[2], UID_GPS_alt);
    sendRadioData( altitude, UID_altitude);
    sendRadioData((float) state, UID_state);
}

//Send Acceleration, IMU data, and temperature
void sendTierTwo(float acc_data[], float bar_data[], float *temp_sensor_data, float IMU_data[]){
    sendRadioData(bar_data[1], UID_bar_temp);
    sendRadioData(*temp_sensor_data, UID_temp_temp);
    sendRadioData(IMU_data[0], UID_IMU_yaw);
    sendRadioData(IMU_data[1], UID_IMU_roll);
    sendRadioData(IMU_data[2], UID_IMU_pitch);
    sendRadioData(acc_data[0], UID_acc_acc_x);
    sendRadioData(acc_data[1], UID_acc_acc_y);
    sendRadioData(acc_data[2], UID_acc_acc_z);
}
//Send data that only needs to be sent once, battery voltage, Ground Altitude
void sendTierThree(float* battery_voltage, float* ground_altitude){
    sendRadioData(*ground_altitude, UID_ground_altitude);
    sendRadioData(*battery_voltage, UID_batt);


}

void sendRadioData(float data, char id){
    //teensy should be little endian, which means least significant is stored first, make sure ground station decodes accordingly
     u_int8_t b[4];
      *(float*) b = data;
      SerialRadio.write(id);
      for(int i=0; i<4; i++)
      {
          SerialRadio.write(b[i]);
      }
}