/*
  Copyright (c) 2022 FirstBuild
  NM 2025.03.09 updated PARAMETER_MIN_ANALOG_OUTPUT to PARAMETER_MIN_ANALOG_RH_OUTPUT in 3 locations to create a min output that will support a fogger
   -- this will just keep the fans always going at 170... we need a way to shut them off as well
*/

#include "HumidityController.h"
#include <Arduino.h>
#include "Macros.h"
#include "Parameters.h"
#include "PinDefinitions.h"
#include <FastPID.h>
#include "Encoders.h"
#include "Timers.h"
#include "Logging.h"

#define FULL_HUMIDITY (100)
#define INTEGRATOR_WINDUP_DETECTION_SECONDS (15)
#define INTEGRATOR_WINDUP_DETECTION_COUNT_MAX ((INTEGRATOR_WINDUP_DETECTION_SECONDS * MS_PER_SEC) / PARAMETER_APPLICATION_RUN_DELAY_MS)


// Define states for the fan
typedef enum {
   HUM_STATE_OFF,
   HUM_STATE_ON
} HumState_t;

static HumidityController_t instance;
static FastPID humidityPid(PARAMETER_HUMIDITY_PID_KP, PARAMETER_HUMIDITY_PID_KI, PARAMETER_HUMIDITY_PID_KD, (MS_PER_SEC / PARAMETER_APPLICATION_RUN_DELAY_MS), 8, false);
static uint32_t lastCycleMillis = 0;
static HumState_t humState = HUM_STATE_OFF;  //start in OFF state 
static uint32_t currentMillis = 0;
static uint32_t onTime = PARAMETER_HUMIDITY_PERIOD_SEC * MS_PER_SEC;    // initialize at 50% duty cycle
static uint32_t offTime = PARAMETER_HUMIDITY_PERIOD_SEC * MS_PER_SEC;   // initialize at 50% duty cycle
static uint32_t totalCycleTime = PARAMETER_HUMIDITY_PERIOD_SEC * MS_PER_SEC;  

static void UpdateSetpointFromKnob(void)
{
   uint8_t knobValue = Encoders_GetHumiditySetting();

   if (knobValue == ENCODER_MIN_POSITION)
   {
      instance._private.setPoint = 0;
   }
   else if (PARAMETER_HUMIDITY_MODE  ==  HUM_MODE_DUTY ){
      instance._private.setPoint = map(
         knobValue,
         (ENCODER_MIN_POSITION + 1),
         ENCODER_MAX_POSITION,
         1,
         PARAMETER_HUMIDITY_MAX_DUTY_CYCLE); // 1-100% duty cycle
   }
   else
   {
      instance._private.setPoint = map(
          knobValue,
          (ENCODER_MIN_POSITION + 1),
          ENCODER_MAX_POSITION,
          PARAMETER_HUMIDITY_MIN_SETPOINT,
          PARAMETER_HUMIDITY_MAX_SETPOINT);
   }
}

static void UpdateReadingFromSensor(void)
{
   instance._private.sensor.reset();
   instance._private.sensorValue = instance._private.sensor.readHumidity();
   instance._private.tempValue = instance._private.sensor.readTemperature();
   instance._private.heatStatus = instance._private.sensor.isHeaterEnabled();

}

static String GetStatusAsString(void)
{
   return (instance._private.heatStatus == true) ? "Enabled" : "Disabled";
}

static void ResetPid(void)
{
   humidityPid.clear();
   humidityPid.configure(PARAMETER_HUMIDITY_PID_KP, PARAMETER_HUMIDITY_PID_KI, PARAMETER_HUMIDITY_PID_KD, (MS_PER_SEC / PARAMETER_APPLICATION_RUN_DELAY_MS), 8, false);
   humidityPid.setOutputRange(PARAMETER_MIN_ANALOG_OUTPUT, PARAMETER_MAX_ANALOG_OUTPUT);

   if (humidityPid.err())
   {
      Logging_Verbose("PID Config Error");
   }
}

/**
 * @brief This function checks for positive integrator windup by seeing if the PID output
 * is saturated at the max value while the error (setPoint - actualValue) is negative for
 * some predetermined amount of time (i.e. 15 seconds). If postive windup is detected,
 * reset the PID so it can start operating again.
 *
 */
static void PreventPositiveIntegratorWindup(void)
{
   if ((instance._private.pidRequest == PARAMETER_MAX_ANALOG_OUTPUT) && (instance._private.setPoint < instance._private.sensorValue))
   {
      if (instance._private.integratorPositiveWindupCounter++ >= INTEGRATOR_WINDUP_DETECTION_COUNT_MAX)
      {
         ResetPid();
         instance._private.integratorPositiveWindupCounter = 0;
      }
   }
   else
   {
      instance._private.integratorPositiveWindupCounter = 0;
   }
}

/**
 * @brief This function checks for negative integrator windup by seeing if the PID output
 * is saturated at the min value while the error (setPoint - actualValue) is positive for
 * some predetermined amount of time (i.e. 15 seconds). If negative windup is detected,
 * reset the PID so it can start operating again.
 *
 */
static void PreventNegativeIntegratorWindup(void)
{
   if ((instance._private.pidRequest == PARAMETER_MIN_ANALOG_OUTPUT) && (instance._private.setPoint > instance._private.sensorValue))
   {
      if (instance._private.integratorNegativeWindupCounter++ >= INTEGRATOR_WINDUP_DETECTION_COUNT_MAX)
      {
         ResetPid();
         instance._private.integratorNegativeWindupCounter = 0;
      }
   }
   else
   {
      instance._private.integratorNegativeWindupCounter = 0;
   }
}

 // Calculate new ON and OFF durations based on Duty Cycle
 static void UpdateHumState(void) {
   onTime =  (uint32_t)((float)(totalCycleTime) * (instance._private.setPoint / 100.0));
   offTime = totalCycleTime - onTime;

   if (humState == HUM_STATE_OFF) {
       if (currentMillis >= offTime) {
           humState = HUM_STATE_ON;
           currentMillis = 0;
           Logging_Verbose_1("Humidifier turned ON for %lu sec", onTime/MS_PER_SEC);
       }
       else{currentMillis = currentMillis + PARAMETER_APPLICATION_RUN_DELAY_MS;}
   } 
   else if (humState == HUM_STATE_ON) {
       if (currentMillis >= onTime) {
           humState = HUM_STATE_OFF;
           currentMillis = 0;
           Logging_Verbose_1("Humidifier turned OFF for %lu sec", offTime/MS_PER_SEC);
       }
       else{currentMillis = currentMillis + PARAMETER_APPLICATION_RUN_DELAY_MS;}
   }
}

static String GetStatusAsString2(void)
{
   return (humState == HUM_STATE_ON) ? "Enabled" : "Disabled";
}

static uint16_t GetStatusTimeRemaining(void)
{
   uint32_t elapsed = currentMillis;
   uint32_t remaining = 0;

   if (humState == HUM_STATE_ON) {
         remaining = (onTime > elapsed) ? (onTime - elapsed) : 0;
   } else {
         remaining = (offTime > elapsed) ? (offTime - elapsed) : 0;
   }

   return remaining / MS_PER_SEC;
}

static void CalculateFanOutput(void)
{
   //IF in DUTY CYCLE MODE check what part of cycle you are in
   if(PARAMETER_HUMIDITY_MODE  ==  HUM_MODE_DUTY ){
      if(humState == HUM_STATE_ON)
      {
         digitalWrite(HUMIDITY_OUTPUT_PIN, HIGH);
      }
      else
      {
         digitalWrite(HUMIDITY_OUTPUT_PIN, LOW);
      }
   }
   
   // else Shipped configuration
   else {  
      instance._private.pidRequest = humidityPid.step(instance._private.setPoint, instance._private.sensorValue);

      if (0 == instance._private.setPoint)
      {
         ResetPid();
         instance._private.outputValue = 0;
      }
      else if (instance._private.pidRequest <= PARAMETER_HUMIDITY_MINIMUM_OUTPUT)
      {
         instance._private.outputValue = PARAMETER_MIN_ANALOG_OUTPUT;
         PreventNegativeIntegratorWindup();
      }
      else
      {
         instance._private.outputValue = instance._private.pidRequest;
         PreventPositiveIntegratorWindup();
      }

      analogWrite(HUMIDITY_OUTPUT_PIN, instance._private.outputValue);
   } 
}

static void SetupPwmOutput(void)
{
   //set up PWM if in PWM Mode
   if(PARAMETER_HUMIDITY_MODE  == HUM_MODE_PWM){
      // Configure Timer 3
      TCCR3A = 0b00000001; // Phase Correct PWM Mode, 8-bit, TOP = 0xFF
      TCCR3B = 0b00000100; // Divide I/O clock by 256 (8MHz / 256 = 31,250 Hz)
   }
   //Either way set up output pin
   pinMode(HUMIDITY_OUTPUT_PIN, OUTPUT);
}

static void SetupHumiditySensor(void)
{
   Wire.begin();
   instance._private.sensor = Adafruit_SHT31();
   instance._private.sensor.begin();
}

bool HumidityController_ValueIsNormal(void)
{
   return (instance._private.setPoint == 0) ||
          VALUE_IN_BOUNDS(
              instance._private.sensorValue,
              instance._private.setPoint - PARAMETER_HUMIDITY_ERROR_OFFSET,
              FULL_HUMIDITY);
}

void HumidityController_LogHeader(void)
{
   Logging_Info_Data("RH Setting, ");
   Logging_Info_Data("RH Read, ");
   Logging_Info_Data("F Read, ");
   Logging_Info_Data("SHT31 Heater, ");
   Logging_Info_Data("Fogger Status, ");
   Logging_Info_Data("Status Remain, ");
   //Logging_Info_Data("Uptime, ");
   Logging_Info_Data("RH Output, ");
   Logging_Info_Data("RH PID Out, ");
   Logging_Info_Data("RH -ISat, ");
   Logging_Info_Data("RH +ISat, ");
   Logging_Info_Data("|, ");
}

void HumidityController_LogInfo(void)
{
   Logging_Info_Data_1("%6d %%RH, ", int(instance._private.setPoint));
   Logging_Info_Data_1("%3d %%RH, ", int(instance._private.sensorValue));
   Logging_Info_Data_1("%3d F, ", int(instance._private.tempValue));
   Logging_Info_Data_1("%9s, ", GetStatusAsString().c_str());
   Logging_Info_Data_1("%9s, ", GetStatusAsString2().c_str());
   Logging_Info_Data_1("%5u sec, ", GetStatusTimeRemaining());
   //Logging_Info_Data_1("%6lu ms, ", millis());   // all references to millis were removed, this was causing timer issues and the system would freeze
   Logging_Info_Data_1("%3d Fan Command, ", instance._private.outputValue);
   Logging_Info_Data_1("%3d PID Output,", instance._private.pidRequest);
   Logging_Info_Data_1("%3d -ISat Count, ", instance._private.integratorNegativeWindupCounter);
   Logging_Info_Data_1("%3d +ISat Count, ", instance._private.integratorPositiveWindupCounter);
   Logging_Info_Data("|, ");
}

void HumidityController_Run(void)
{
   UpdateSetpointFromKnob();
   UpdateReadingFromSensor();
   if(PARAMETER_HUMIDITY_MODE  ==  HUM_MODE_DUTY ){UpdateHumState();}
   CalculateFanOutput();
}

void HumidityController_Init(void)
{
   SetupPwmOutput();
   SetupHumiditySensor();
   ResetPid();
   humState = HUM_STATE_ON;     // Start in ON state
   currentMillis = 0;
   instance._private.integratorNegativeWindupCounter = 0;
   instance._private.integratorPositiveWindupCounter = 0;
   instance._private.pidRequest = 0;
   instance._private.outputValue = 0;
}
