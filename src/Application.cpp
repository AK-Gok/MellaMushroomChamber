/*
  Copyright (c) 2022 FirstBuild
*/

#include "Application.h"
#include "UnitState.h"
#include "Encoders.h"
#include "Logging.h"
#include "MellaVersion.h"
#include "Timers.h"

static Application_t instance;

void Application_Tick(void)
{
   static unsigned long lastRun = 0;
   static unsigned long lastLog = 0;
   unsigned long now = millis();

   if (now - lastRun >= PARAMETER_APPLICATION_RUN_DELAY_MS) {
      lastRun = now;
      Application_Run();
   }

   if (now - lastLog >= PARAMETER_APPLICATION_LOG_DELAY_MS) {
      lastLog = now;
      Application_Log();
   }
}

static void LogVersionHeader(void)
{
   Logging_Info_Data("     Build, ");
   Logging_Info_Data("|, ");
}

static void LogVersionData(void)
{
   Logging_Info_Data_1("%10s, ", MELLA_BUILD_VERSION);
   Logging_Info_Data("|, ");
}

static void Application_LogHeaders(void)
{
   Encoders_LogHeader();
   UnitState_LogHeader();
   LightController_LogHeader();
   HumidityController_LogHeader();
   AirExchangeController_LogHeader();
   LogVersionHeader();
   Logging_Info("");
}

void Application_Log(void)
{
   static uint8_t logCounter = 0;

   if((++logCounter % 60) == 0)
   {
      Application_LogHeaders();
      logCounter = 0;
   }

   Encoders_LogInfo();
   UnitState_LogInfo();
   LightController_LogInfo();
   HumidityController_LogInfo();
   AirExchangeController_LogInfo();
   LogVersionData();
   Logging_Info("");
}

void Application_Run(void)
{
   Encoders_Run();
   AirExchangeController_Run();
   HumidityController_Run();
   UnitState_Run();
   LightController_Run();
}

void Application_Init(void)
{
   delay(5000);
   Logging_Info("Initializing Application...");
   Logging_Verbose("Timing...");
   Timers_Init();

   Logging_Verbose("Encoders...");
   Encoders_Init();

   Logging_Verbose("Air Exchange Controller...");
   AirExchangeController_Init();

   Logging_Verbose("Humidity Controller...");
   HumidityController_Init();

   Logging_Verbose("Unit State...");
   UnitState_Init();

   Logging_Verbose("Light Controller...");
   LightController_Init();

   Logging_Verbose("Recurring Timers...");
   Timers_SetupPeriodicTimer(TimerId_Application, Application_Run, PARAMETER_APPLICATION_RUN_DELAY_MS);
   Timers_SetupPeriodicTimer(TimerId_Logging, Application_Log, PARAMETER_APPLICATION_LOG_DELAY_MS);
   Logging_Info("Application Initialization Complete");
   Application_LogHeaders();
}
