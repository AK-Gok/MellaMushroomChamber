/*
  Copyright (c) 2022 FirstBuild
*/

#include "LightController.h"
#include "PinDefinitions.h"
#include "Parameters.h"
#include "Encoders.h"
#include "Logging.h"

static LightController_t instance;
static uint32_t animationStep = 0;
static bool lastHeartbeartState = LOW;
static uint16_t lastHeartbeartSec = 0;

// variables for timer
// Define states for the fan
typedef enum {
   LIGHT_STATE_OFF,
   LIGHT_STATE_ON
} LightState_t;

// Use macro to ensure all math is done in 64 bits and prevent overflow
#define HOURS_TO_MS(hours) ((uint32_t)(((uint64_t)(hours)) * 60ULL * 60ULL * 1000ULL))
static const uint32_t onTime = HOURS_TO_MS(PARAMETER_LIGHTS_HOURS_ON);    // ms on time
static const uint32_t offTime = HOURS_TO_MS(HRS_PER_DAY - PARAMETER_LIGHTS_HOURS_ON);   // ms off time
static LightState_t lightState = LIGHT_STATE_ON;  //start in ON state

static uint32_t lastTransitionMillis = 0; // shared between UpdateLightState and GetStatusTimeRemaining
static int prevKnobValue = -1;

// Helper for drift-compensated millis. -- 10/5/25 using this function caused a crash within 24hrs
static inline uint32_t driftedMillis() {
   return (uint32_t)(millis() * PARAMETER_MILLIS_DRIFT_FACTOR);
}

void LightController_SetMode(LightMode_t mode)
{
   if(mode < LightMode_Max)
   {
      instance._private.mode = mode;
      animationStep = 0;
   }
}

static void GetBrightnessFromKnob(void)
{
   instance._private.brightness = map(
      Encoders_GetLightSetting(),
      ENCODER_MIN_POSITION,
      ENCODER_MAX_POSITION,
      0,
      100);
}


static bool GetNormalHeartbeatLedState(void)
{
   uint16_t sec = (millis() / MS_PER_SEC);
   //uint16_t sec = (driftedMillis() / MS_PER_SEC);
   if(sec >= (lastHeartbeartSec + (PARAMETER_LIGHTS_HEARTBEAT_NORMAL_DELAY_MS / MS_PER_SEC)))
   {
      lastHeartbeartSec = sec;
      lastHeartbeartState = !lastHeartbeartState;
   }
   return (lastHeartbeartState);
}

static bool GetFaultHeartbeatLedState(void)
{
   uint16_t sec = (millis() / MS_PER_SEC);
   //uint16_t sec = (driftedMillis() / MS_PER_SEC);
   if(sec >= lastHeartbeartSec + (PARAMETER_LIGHTS_HEARTBEAT_FAULT_DELAY_MS / MS_PER_SEC))
   {
      lastHeartbeartSec = sec;
      lastHeartbeartState = !lastHeartbeartState;
   }
   return (lastHeartbeartState);
}

// Update State based on Light Time and knob setting
static void UpdateLightState(void) {
   // prevKnobValue is now file scope
   uint32_t now = millis();
   int knobValue = Encoders_GetLightSetting();

   if (knobValue == 0) {
      lastTransitionMillis = now;
      if (lightState != LIGHT_STATE_OFF) {
         lightState = LIGHT_STATE_OFF;
         //Logging_Verbose("Lights forced OFF by knob");
      }
   } else {
      // If knob was 0 and now >0, turn ON and restart timer
      if (prevKnobValue == 0) {
         lightState = LIGHT_STATE_ON;
         lastTransitionMillis = now;
         //Logging_Verbose("Lights turned ON by knob change");
      }
      if (lightState == LIGHT_STATE_ON) {
         if (now - lastTransitionMillis >= onTime) {
            lightState = LIGHT_STATE_OFF;
            lastTransitionMillis = now;
            //Logging_Verbose_1("Lights turned OFF for %lu hrs", offTime/MS_PER_SEC/SEC_PER_MIN/MIN_PER_HR);
         }
      } else if (lightState == LIGHT_STATE_OFF) {
         if (now - lastTransitionMillis >= offTime) {
            lightState = LIGHT_STATE_ON;
            lastTransitionMillis = now;
            //Logging_Verbose_1("Lights turned ON for %lu hrs", onTime/MS_PER_SEC/SEC_PER_MIN/MIN_PER_HR);
         }
      }
   }
   prevKnobValue = knobValue;
}

static String GetStatusAsString(void)
{
   return (lightState == LIGHT_STATE_ON) ? "Enabled" : "Disabled";
}

static uint32_t GetStatusTimeRemaining(void)
{
   
   // Use the same static variable as UpdateLightState
   
   uint32_t now = millis();
   uint32_t remaining = 0;
   int knobValue = Encoders_GetLightSetting();
   if (knobValue == 0) {
      return 99999; // special indicator Timer not running, light forced off
   }
   if (lightState == LIGHT_STATE_ON) {
      remaining = (onTime > (now - lastTransitionMillis)) ? (onTime - (now - lastTransitionMillis)) : 0;
   } else {
      remaining = (offTime > (now - lastTransitionMillis)) ? (offTime - (now - lastTransitionMillis)) : 0;
   }
   return remaining / MS_PER_SEC;
}



static void RunNormalMode(void)
{
  //use light state to override knob setting if enters LIGHT_STATE_OFF 
   if (lightState==LIGHT_STATE_ON){  
      instance._private.output = ((0 == instance._private.brightness) ? 0 : map(instance._private.brightness, 1, 100, PARAMETER_LIGHTS_MIN_SETPOINT, PARAMETER_LIGHTS_MAX_SETPOINT));
   }
   else {instance._private.output = 0;
   }
   analogWrite(LIGHT_OUTPUT_PIN, instance._private.output);
   digitalWrite(ON_BOARD_LED_PIN, GetNormalHeartbeatLedState());
}

static uint8_t GetSawtoothBrightness(void)
{
   int32_t retVal = map((millis() % PARAMETER_LIGHTS_FAULT_BREATHE_PERIOD_MS), 0, PARAMETER_LIGHTS_FAULT_BREATHE_PERIOD_MS, PARAMETER_MIN_ANALOG_OUTPUT, PARAMETER_MAX_ANALOG_OUTPUT * 2);
   retVal = (retVal <= PARAMETER_MAX_ANALOG_OUTPUT) ? retVal : (PARAMETER_MAX_ANALOG_OUTPUT - retVal);
   return int(retVal);
}

static void RunFaultMode(void)
{
   instance._private.output = GetSawtoothBrightness();
   analogWrite(LIGHT_OUTPUT_PIN, instance._private.output);
   digitalWrite(ON_BOARD_LED_PIN, GetFaultHeartbeatLedState());
}

static bool IsInFaultMode(void)
{
   return instance._private.mode == LightMode_Fault;
}

void LightController_LogHeader(void)
{
   Logging_Info_Data("LL Setting, ");
   Logging_Info_Data("LL Mode, ");
   Logging_Info_Data("LL Output, ");
   Logging_Info_Data("LL Timer, ");
   Logging_Info_Data("|, ");
}

void LightController_LogInfo(void)
{
   Logging_Info_Data_1("%7u %%, ", (uint16_t)map(instance._private.output, 0, PARAMETER_MAX_ANALOG_OUTPUT, 0, 100));

   if(IsInFaultMode())
   {
      Logging_Info_Data("  Fault, ");
   }
   else
   {
      Logging_Info_Data("  Normal, ");
   }

   Logging_Info_Data_1("%7li %%, ", map(instance._private.output, 0, PARAMETER_MAX_ANALOG_OUTPUT, 0, 100));
   Logging_Info_Data_1("%7lu sec, ", GetStatusTimeRemaining());
   Logging_Info_Data("|, ");
}

void LightController_Run(void)
{
   GetBrightnessFromKnob();
   if(instance._private.mode == LightMode_Fault)
   {
      RunFaultMode();
   }
   else
   {
      if(PARAMETER_LIGHTS_MODE == LIGHT_TIMER_ON) {UpdateLightState();}
      RunNormalMode();
   }
}

void LightController_Init(void)
{
   pinMode(LIGHT_OUTPUT_PIN, OUTPUT);
   pinMode(ON_BOARD_LED_PIN, OUTPUT);
   LightController_SetMode(LightMode_Normal);
/*
   // Debug: Log timing parameters and calculated times
   Logging_Info_1("PARAMETER_LIGHTS_HOURS_ON: %lu", (unsigned long)PARAMETER_LIGHTS_HOURS_ON);
   Logging_Info_1("HRS_PER_DAY: %lu", (unsigned long)HRS_PER_DAY);
   Logging_Info_1("onTime (ms): %lu", (unsigned long)onTime);
   Logging_Info_1("offTime (ms): %lu", (unsigned long)offTime);
   Logging_Info_1("onTime (sec): %lu", (unsigned long)(onTime/1000));
   Logging_Info_1("offTime (sec): %lu", (unsigned long)(offTime/1000));
*/
}
