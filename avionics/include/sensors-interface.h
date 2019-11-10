#ifndef SENSOR_INTERFACE_H
#define SENSOR_INTERFACE_H

enum class SensorStatus {
    NOMINAL,
    FAILURE
};

/**
 * @brief enum of all sensors
 */
enum SensorType {
    ACCELEROMETER,
    BAROMETER,
    THERMOMETER,
    IMU,
    GPS,
    THERMOCOUPLE
};

class ISensor {
    public:
    /**
      * @brief  Initialize sensor
      * @return SensorStatus 
      */
    virtual SensorStatus initSensor() = 0;

    /** 
      * @brief  Reads sensor data
      */
    virtual void readData() = 0;

    /**
     * @brief Returns the length of the data array the sensor requires
     */
    virtual uint8_t dataLength() = 0;

    /**
     * @brief Returns data read during readData()
     * @return the data
     */
    virtual float* getData() = 0;

    /**
     * @brief returns the current status of the snesor
     * @return the current sensor status
     */
    virtual SensorStatus getStatus() = 0;
};
#endif