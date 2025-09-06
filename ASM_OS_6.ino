/********************************************************* NOTES ********************************************************/ /*
1. Should task variables be defined inside the fucntion, or inside if statements, or global? 
    What is their life and where and how are they stored?
    How to test??? Stored in global? In stack? How can I see if it is moved to stack?
2. OS can crash before serial buffer is complete
3. Every task function must have an endless loop - for (;;) {}

Timer notes:
  TIMER 0 - 8 Bit.   micros, millis, other PWM? - interrupt. PWM 5,6
            Analog out 5,6
            PS 64, Tick 4uS  0.000,004 S OVFC 255 (1006 uS?)
  TIMER 1 - 16 Bit.  Servo - running interrupt               PWM 9.10
            PS 8,  Tick 0.5uS 0.000,000,5 S OVFC 40,000 (10 mS)
  TIMER 2 - 8 Bit,   ISR for RTOS - 1 ms, also yields.       PWM 11,3
            PS 64, Tick 6uS  0.000,004 S OVFC 250 (1mS)
  PWM     - Runs independently????

Timing Notes:
  16 mhZ - 0.000,000,06 S
  Scheduler timing - 100 opcodes is about 6uS

Pin and schematic layout:
Pin  Program   Hardware Board
0  -                     RXD
1  -                     TXD
3  -                     Timer2B
4  -  
5  -                     Timer0A   analogout()
6  -                     Timer0B   analogout()
7  -             
8  -            
9  -                    Timer1A
10 -                    Timer1B
11 -                    Timer2A
12 -             
13 -                    LED
18 -                    SDA
19 -                    SCL

Library search order: Board specific, Skethch Folder, IDE
"library" Looks in Sketch Folder first
/**************************************************************************************************************************************************/

#include <Arduino.h>  // Built-ins, <avr/pgmspace.h>, <avr/io.h>, <math.h>, <avr/interrupt.h>
#include "ASM_OS.h"
#include <EEPROM.h>
#include <avr/wdt.h>
#include "uart.h"
#include "tasks.h"
#include "game.h"
#include "fs.h"
#include "ss.h"
#include <avr/boot.h>

#define O_BOARD_LED 13  // Onboard LED pin on Arduino Uno

// void setupWDT() {
//   MCUSR = 0;                           // clear reset flags
//   WDTCSR |= (1 << WDCE) | (1 << WDE);  // Set WDT to interrupt mode only, ~16 ms
//   WDTCSR = (1 << WDIE) | (1 << WDP0);  // WDP0=1, WDP1=0, WDP2=0 -> 16 ms
// }

// ISR(WDT_vect) {
//   SP = (uint16_t)tasks[0].stack + tasks[0].stackSize - 1;
//   send(F("\r\nWDT\r\n"));
//   cli();
//   ps(); //print process status for debug (HWM)
//   // WDTCSR = 0;
//   for (;;) { }
// }

void serialShellTask() {
  delay(300);
  sflush();
  loadPSInitFromFS();
  sflush();
  send(F("\r\n*************************"));
  send(F("\r\n'?' for a list of commands.\r\n\n"));
  send(F("> "));
  for (;;) {
    if (SS_RecvC == true) {
      SS_RecvC = 0;
      SS_Process_Command();
    }
    if (printTimeInterval) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastTimePrint >= 15000) {
        lastTimePrint = currentMillis;

        unsigned long seconds = currentMillis / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;
        seconds %= 60;
        minutes %= 60;
        hours %= 24;

        send(F("\r\n>>[Time] "));
        send(hours);
        send(F("h "));
        send(minutes);
        send(F("m "));
        send(seconds);
        send(F("s\r\n> "));
      }
    }
    if (gameOn) {
      unsigned long currentTime = millis();
      if (currentTime - lastUpdate > updateInterval) {
        lastUpdate = currentTime;
        moveObstaclesDown();
        spawnObstacle();
        checkCollision();
        printGrid();
      }
    }
    wdt_reset();
    yield();
  }
}

void SS_Process_Command() {
  if (gameOn != 1) {
    // send((const char *)command_buffer);
  }
  if (strcmp(command_buffer, "?") == 0) {
    help();
  } else if (gameOn == 1) {
    if (strcmp(command_buffer, "a") == 0) {
      if (playerCol > 0) playerCol--;
    } else if (strcmp(command_buffer, "s") == 0) {
      if (playerCol < cols - 1) playerCol++;
    } else if (strcmp(command_buffer, "x") == 0)
      gameOn = 0;
  } else if (strcmp(command_buffer, "cg") == 0) {
    game();
  } else if (strcmp(command_buffer, "pt") == 0) {
    time();
  } else if (strcmp(command_buffer, "ps") == 0) {
    ps();
  } else if (strcmp(command_buffer, "ft") == 0) {
    FS_format();
  } else if (strcmp(command_buffer, "ls") == 0) {
    FS_list();
  } else if (strncmp(command_buffer, "rd ", 3) == 0) {
    FS_read(command_buffer + 3);
  } else if (strncmp(command_buffer, "wr ", 3) == 0) {
    wr();
  } else if (strncmp(command_buffer, "rm ", 3) == 0) {
    FS_delete(command_buffer + 3);
  } else {
    send(F("\r\n>> Unknown command. Type '?' for a list of commands."));
  }
  if (gameOn == 0) {
    send(F("\r\n> "));
  }
}

void setup() {
  //cli();   //conflicting with Wire.Begin
  uart_init(115200);
  send(F("\r\n\n\n*************************\r\nBooting AtMega328P"));
  sflush();
  send(F("\r\n Clock: "));
  send((int)(F_CPU / 1000000));
  send(F(" MHz\r"));
  uint8_t sig1 = boot_signature_byte_get(0);
  uint8_t sig2 = boot_signature_byte_get(1);
  uint8_t sig3 = boot_signature_byte_get(2);
  send(F("\r\n Boot sig: "));
  sendh(sig1);
  send(" ");
  sendh(sig2);
  send(" ");
  sendh(sig3);
  sflush();
  VT_Initialize();
  loadOSInit();
  sflush();
  send(F("\r\n*************************"));

  createTask(serialShellTask, 0);
  createTask(stackMonitorTask, 1);
  // createTask(SV_Task, 2);
  // createTask(MM_Task, 3);
  // createTask(YW_Task, 4);
  // createTask(US_Task, 5);
  // createTask(NP_Task, 6);

  // setupWDT();

  send(F("\r\nStarting kernel"));
  StartOS();
  for (;;) {}
}

void loop() {
  for (;;) {}
}
