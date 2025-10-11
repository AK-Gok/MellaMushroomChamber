/*
  Copyright (c) 2022 FirstBuild
*/

#ifndef PARAMETERS_H
#define PARAMETERS_H

/***********************************************************************************************************************************************************************************************************************************************************************/
// define high level control parameters
#define PARAMETER_LOG_LEVEL                         LogLevel_Info     // LogLevel_Debug, LogLevel_Verbose, LogLevel_Info, LogLevel_Off

#define PARAMETER_STATE_MODE                        STATE_IGNORE      // STATE_IGNORE, STATE_ON | STATE_IGNORE(will ignore humidity and stay in normal state -- NO FLASHING LIGHTS), STATE_ON (factory shipped settings)
#define PARAMETER_HUMIDITY_MODE                     HUM_MODE_SENSOR   // HUM_MODE_DUTY, HUM_MODE_SENSOR | HUM_MODE_DUTY cycles humidiier on and off, Knob Value = Duty Cycle (also disables light flashing), HUM_MODE_SENSOR (uses SHT31 sensor to control humidity) 
#define PARAMETER_HUMIDITY_DEVICE                   HUM_DEV_FAN       // HUM_DEV_FAN, HUM_DEV_FOG | FAN configures output pin for PWM, FOG configures pin to be High/Low, not yet supported with HUM_MODE_SENSOR (FOGGER in testing, for future release) 
#define PARAMETER_HUMIDITY_MAX_DUTY_CYCLE           (100)              // integer 0-100 | % duty cycle for humidifier when in HUM_MODE_DUTY  
#define PARAMETER_HUMIDITY_WIND_REGULATOR           (float(0.42))     // float between 0-1 | Lower the max humidifier fan speed to prevent wind burn (my target 5-6v provided equal humidity with less wind burn)
#define PARAMETER_LIGHTS_MODE                       LIGHT_TIMER_ON    // LIGHT_TIMER_ON, LIGHT_TIMER_OFF | This will turn on an off lights in a 24hr period starting/resetting when the knob is turned on from 0
#define PARAMETER_LIGHTS_HOURS_ON                   (12)              // integer 1-24 | Number of hours lights should be on within a 24hr period
/***********************************************************************************************************************************************************************************************************************************************************************/


#define PARAMETER_MAX_ANALOG_OUTPUT                 (255)
#define PARAMETER_MIN_ANALOG_OUTPUT                 (0)
#define PARAMETER_APPLICATION_RUN_DELAY_MS          (100)
#define PARAMETER_APPLICATION_LOG_DELAY_MS          (1000)
#define PARAMETER_FAN_STALL_SPEED                   (20)

#define PARAMETER_HUMIDITY_MIN_SETPOINT             (50)
#define PARAMETER_HUMIDITY_MAX_SETPOINT             (82)
#define PARAMETER_HUMIDITY_PID_KP                   (float(5))
#define PARAMETER_HUMIDITY_PID_KI                   (float(0.2))
#define PARAMETER_HUMIDITY_PID_KD                   (float(0.001))
#define PARAMETER_HUMIDITY_ERROR_OFFSET             (12)
#define PARAMETER_HUMIDITY_MINIMUM_OUTPUT           (2 * PARAMETER_FAN_STALL_SPEED) 
#define PARAMETER_HUMIDITY_MAX_ANALOG_OUTPUT        (static_cast<int>(PARAMETER_MAX_ANALOG_OUTPUT * PARAMETER_HUMIDITY_WIND_REGULATOR)) //Limit the max fan speed to reduce wind burn
#define PARAMETER_HUMIDITY_PERIOD_SEC               ((uint32_t)600) //600sec = 10min periods for duty cycle calculation        
        

#define PARAMETER_AIR_EXCHANGE_MIN_SETPOINT         (0)
#define PARAMETER_AIR_EXCHANGE_MAX_SETPOINT         (100)
#define PARAMETER_AIR_EXCHANGE_FAN_MIN_OUTPUT       (2 * PARAMETER_FAN_STALL_SPEED)
#define PARAMETER_AIR_EXCHANGE_FAN_MAX_OUTPUT       (255)
#define PARAMETER_AIR_EXCHANGE_FAN_RAMP_STEP        (1)
#define PARAMETER_AIR_EXCHANGE_FAN_PERIOD_SEC       ((uint32_t)1800)
#define PARAMETER_AIR_EXCHANGE_FAN_DUTY_CYCLE       (100) // Should be a value between 0-100 representing the % duty cycle
#define PARAMETER_AIR_EXCHANGE_FAN_ON_DURATION_SEC  ((PARAMETER_AIR_EXCHANGE_FAN_PERIOD_SEC * PARAMETER_AIR_EXCHANGE_FAN_DUTY_CYCLE) / 100)
#define PARAMETER_AIR_EXCHANGE_FAN_OFF_DURATION_SEC ((PARAMETER_AIR_EXCHANGE_FAN_PERIOD_SEC * (100 - PARAMETER_AIR_EXCHANGE_FAN_DUTY_CYCLE)) / 100)

#define PARAMETER_LIGHTS_MIN_SETPOINT               (10)
#define PARAMETER_LIGHTS_MAX_SETPOINT               (255)
#define PARAMETER_LIGHTS_FAULT_BREATHE_PERIOD_MS    (10000)
#define PARAMETER_LIGHTS_HEARTBEAT_NORMAL_DELAY_MS  (1000)
#define PARAMETER_LIGHTS_HEARTBEAT_FAULT_DELAY_MS   (100)
#define PARAMETER_MILLIS_DRIFT_FACTOR               (float(0.997)) // Adjust based on your measurement of drift (varies with Temperature)


#define PARAMETER_STEADYSTATE_FAULT_DELAY_SEC       (600)
#define PARAMETER_STARTUP_DELAY_SEC                 (1800)
#define PARAMETER_STARTUP_FAULT_DELAY_SEC           (PARAMETER_STARTUP_DELAY_SEC - PARAMETER_STEADYSTATE_FAULT_DELAY_SEC)

#define MS_PER_SEC                                  (1000)
#define SEC_PER_MIN                                 (60)
#define MIN_PER_HR                                  (60)
#define HRS_PER_DAY                                 (24)

#if (MS_PER_SEC % PARAMETER_APPLICATION_RUN_DELAY_MS) != 0
#error "ERROR: (MSEC_PER_SEC / PARAMETER_APPLICATION_RUN_DELAY_MS) must result in an integer"
#endif

#if (PARAMETER_STEADYSTATE_FAULT_DELAY_SEC > PARAMETER_STARTUP_DELAY_SEC)
#error "ERROR: PARAMETER_STEADYSTATE_FAULT_DELAY_SEC cannot be greater than PARAMETER_STARTUP_DELAY_SEC"
#endif

#endif   // PARAMETERS_H
