/*
  Copyright (c) 2022 FirstBuild
*/

#include "Application.h"

// this section will help debug reset issue
#include <avr/wdt.h>
#include <avr/io.h>

uint8_t mcusr_mirror __attribute__ ((section (".noinit")));



void getResetCause() {
  if (mcusr_mirror & _BV(WDRF)) {
    Serial.println("RESET: Watchdog Reset");
  }
  if (mcusr_mirror & _BV(BORF)) {
    Serial.println("RESET: Brown-out Reset");
  }
  if (mcusr_mirror & _BV(EXTRF)) {
    Serial.println("RESET: External Reset");
  }
  if (mcusr_mirror & _BV(PORF)) {
    Serial.println("RESET: Power-on Reset");
  }

  // Clear reset flags
  MCUSR = 0;
}

void storeResetCause(void) __attribute__((naked)) __attribute__((section(".init3")));

void storeResetCause(void)
{
  mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
}

// 

void setup(void)
{
  // added section to signal when a reset occurs to debug
  Serial.begin(115200);
  delay(100);
  getResetCause();  // Print reset cause here
  Serial.println("===== BOOTED =====");
  //
  Application_Init();
}

void loop(void)
{
  Application_Tick();
}
