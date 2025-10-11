/*
  Copyright (c) 2022 FirstBuild
*/

#include "Application.h"
#include <EEPROM.h> // EEPROM logging for reset cause

//this section will help debug reset issue
#include <avr/wdt.h>
#include <avr/io.h>

uint8_t mcusr_mirror __attribute__ ((section (".noinit")));



void getResetCause() {
  Serial.print("mcusr_mirror: ");
  Serial.println(mcusr_mirror, BIN);
  // Read last reset cause from EEPROM
  uint8_t eeprom_reset_cause = EEPROM.read(0);
  Serial.print("EEPROM mcusr_mirror: ");
  Serial.println(eeprom_reset_cause, BIN);
  // Print all flags that are set
  if (mcusr_mirror & _BV(WDRF)) {
    Serial.println("RESET: Watchdog Reset (WDRF)");
  }
  if (mcusr_mirror & _BV(BORF)) {
    Serial.println("RESET: Brown-out Reset (BORF)");
  }
  if (mcusr_mirror & _BV(EXTRF)) {
    Serial.println("RESET: External Reset (EXTRF)");
  }
  if (mcusr_mirror & _BV(PORF)) {
    Serial.println("RESET: Power-on Reset (PORF)");
  }
  if (mcusr_mirror == 0) {
    Serial.println("RESET: Unknown or flags cleared");
  }

  // Bit mapping for reference
  Serial.println("Bit mapping: WDRF=10000, BORF=01000, EXTRF=00100, PORF=00010");

  // Clear reset flags
  MCUSR = 0;
}

void storeResetCause(void) __attribute__((naked)) __attribute__((section(".init3")));

void storeResetCause(void)
{
  mcusr_mirror = MCUSR;
  EEPROM.write(0, mcusr_mirror); // Log to EEPROM address 0
  MCUSR = 0;
  wdt_disable();
}

//

void setup(void)
{
  //added section to signal when a reset occurs to debug
  Serial.begin(115200);
  delay(10000); // Wait for host to reconnect after reset
  getResetCause();  // Print reset cause here
  Serial.println("===== BOOTED =====");
  //
  Application_Init();
}

void loop(void)
{
  Application_Tick();
}
