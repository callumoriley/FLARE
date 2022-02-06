/**
 * Overarching object that carries different components.
 */
#pragma once

#include "CSVWriteImpl.h"
#include "CSVwrite.h"
#include "buzzer.h"
#include "calculations.h"
#include "cameras.h"
#include "radio.h"
#include "sensor_collection.h"
#include "state_machine.hpp"
#include "states_config.hpp"

#include "HAL/time.h"

struct Rocket {
  public:
    static constexpr char LOG_FILE_NAME[] = "datalog.csv";

    Rocket()
        : cam(Hal::SerialInst::Camera), datalog(LOG_FILE_NAME),
          init_status(collectStatus(sensors, ignitors)),
          calc(sensors, Hal::now_ms()), config(calc, ignitors, cam),
          state_machine(config.state_map, config.initial_state) {}

    // WARNING - MEMBER ORDER DEPENDENCY
    // https://isocpp.org/wiki/faq/ctors#order-dependency-in-members
    Buzzer buzzer;
    Camera cam;
    SensorCollection sensors;
    IgnitorCollection ignitors;
    CSVWrite<CSVWriteImpl> datalog;
    RocketStatus init_status;
    Calculator calc;
    StateMachineConfig config;
    StateMachine state_machine;
};
class CommandReceiver {
  private:
    Rocket &rocket_;

    enum PacketTypeId {
        BULK_SENSOR = 0x30,
        GPS = 0x04,
        ACCELERATION_X = 0x10,
        ACCELERATION_Y = 0x11,
        ACCELERATION_Z = 0x12,
        PRESSURE = 0x13,
        BAROMETER_TEMPERATURE = 0x14,
        TEMPERATURE = 0x15,
        LATITUDE = 0x19,
        LONGITUDE = 0x1A,
        GPS_ALTITUDE = 0x1B,
        CALCULATED_ALTITUDE = 0x1C,
        STATE = 0x1D,
        VOLTAGE = 0x1E,
        GROUND_ALTITUDE = 0x1F
    };

  public:
    CommandReceiver(Rocket &rocket) : rocket_(rocket) {}

    void run_command(Radio::command_t command) {
        // Sensor-related data should inform based on the last sensor poll time
        uint32_t sensor_poll_time =
            Hal::tpoint_to_uint(rocket_.sensors.last_poll_time());

        auto state = rocket_.state_machine.getState();
        switch (command) {
        case 'P':
            Radio::sendStatus(Hal::millis(), rocket_.init_status,
                              rocket_.sensors, rocket_.ignitors);
            break;
        case 'A':
            rocket_.state_machine.arm();
            rocket_.cam.start_record();
            break;

        case 'D':
            rocket_.state_machine.disarm();
            rocket_.cam.stop_record();
            break;
        case BULK_SENSOR:
            Radio::sendBulkSensor(
                sensor_poll_time, rocket_.calc.altitude(),
                rocket_.sensors.accelerometer, rocket_.sensors.imuSensor,
                rocket_.sensors.gps, static_cast<uint16_t>(state));
            break;
        case GPS:
            Radio::sendGPS(sensor_poll_time, rocket_.sensors.gps);
            break;
        case ACCELERATION_X:
            Radio::sendSingleSensor(sensor_poll_time, 0x10,
                                    rocket_.sensors.accelerometer.getData()[0]);
            break;

        case ACCELERATION_Y:
            Radio::sendSingleSensor(sensor_poll_time, 0x11,
                                    rocket_.sensors.accelerometer.getData()[1]);
            break;
        case ACCELERATION_Z:
            Radio::sendSingleSensor(sensor_poll_time, 0x12,
                                    rocket_.sensors.accelerometer.getData()[2]);
            break;
        case PRESSURE:
            Radio::sendSingleSensor(sensor_poll_time, 0x13,
                                    rocket_.sensors.barometer.getData()[0]);
            break;
        case BAROMETER_TEMPERATURE:
            Radio::sendSingleSensor(sensor_poll_time, 0x14,
                                    rocket_.sensors.barometer.getData()[1]);
            break;
        case TEMPERATURE:
            Radio::sendSingleSensor(sensor_poll_time, 0x15,
                                    rocket_.sensors.temperature.getData()[0]);
            break;
        case 0x16:
            break;
            // Not implemented - still uncertain about IMU storage
        case 0x17:
            break;
            // Not implemented - still uncertain about IMU storage
        case 0x18:
            break;
            // Not implemented - still uncertain about IMU storage
        case LATITUDE:
            Radio::sendSingleSensor(sensor_poll_time, 0x19,
                                    rocket_.sensors.gps.getData()[0]);
            break;
        case LONGITUDE:
            Radio::sendSingleSensor(sensor_poll_time, 0x1A,
                                    rocket_.sensors.gps.getData()[1]);
            break;
        case GPS_ALTITUDE:
            Radio::sendSingleSensor(sensor_poll_time, 0x1B,
                                    rocket_.sensors.gps.getData()[2]);
            break;
        case CALCULATED_ALTITUDE:
            Radio::sendSingleSensor(sensor_poll_time, 0x1C,
                                    rocket_.calc.altitude());
            break;
        case STATE:
            Radio::sendState(sensor_poll_time, static_cast<uint16_t>(state));
            break;
        case VOLTAGE:
            Radio::sendSingleSensor(sensor_poll_time, 0x1E,
                                    rocket_.sensors.battery.getData()[0]);
            break;
        case GROUND_ALTITUDE:
            Radio::sendSingleSensor(sensor_poll_time, 0x1F,
                                    rocket_.calc.altitudeBase());
            break;
        default:
            // TODO: Log invalid command
            break;
        }
    }
};
