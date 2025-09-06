bool printTimeInterval = false;
bool printSD = false;
unsigned long lastTimePrint = 0;

const char TXT_Help[] PROGMEM =
  "\r\n"
  "  ps\r\n"
  "  ls\r\n"
  "  ft\r\n"
  "  rd <name>\r\n"
  "  wr <name>\r\n"
  "  rm <name>\r\n"
  "  pt\r\n"
  "  cg\r\n"
  "  ?";

void ps() {
  sendl();
  send(F("\r\n*** System Status *************************************************************\r\n   Voltage: "));
  send((float)VT_ReturnVoltage(), 2);
  uint32_t nowSec = millis() / 1000;
  send(F("  Uptime: "));
  send((int)nowSec / 3600);
  send(F("h "));
  send((int)(nowSec % 3600) / 60);
  send(F("m "));
  send((int)nowSec % 60);
  send(F("s"));
  send(F("   Last ISR uS: "));
  send(OS_ISR_Clock);
  send(F("   TimeSlice: "));
  send(OS_TimeSlice);
  send(F("\r\n*** High-water marks **********************************************************"));
  send(F("\r\n   Task\tName\tHWM\tStackBytes\tmS\tmin\tmax\t%"));
  uint32_t cpuTotal = 0;
  for (uint8_t i = 0; i < taskCount; i++) {
    cpuTotal += tasks[i].cpuAccum;
  }
  for (uint8_t i = 0; i < taskCount; i++) {
    float percent = (cpuTotal > 0) ? (100.0f * tasks[i].cpuAccum / cpuTotal) : 0.0f;
    send(F("\r\n   "));
    send(i);
    send(F(":\t"));
    send(taskNames[i]);
    send(F("\t"));
    send(tasks[i].highWaterMark);
    send(F("\t"));
    send(tasks[i].stackSize);
    send(F("\t\t"));
    send(tasks[i].cpuAccum / 1000);
    send(F("\t"));
    send(tasks[i].minSliceUs);
    send(F("\t"));
    send(tasks[i].maxSliceUs);
    send(F("\t"));
    send(percent, 1);
  }
  sendl();
}


void wr() {
  char *p = command_buffer + 3;
  while (*p == ' ') p++;
  char filename[FS_NAME_LEN + 1];
  int i = 0;
  while (*p && *p != ' ' && i < FS_NAME_LEN) {
    filename[i++] = *p++;
  }
  filename[i] = 0;
  send(F("\r\n:"));
  uint8_t inputIndex = 0;
  SS_RecvC = 0;
  while (SS_RecvC == 0) { yield(); }
  send(F("#"));
  send((const char *)command_buffer);
  send(F("#"));
  FS_write(filename, command_buffer);
  SS_RecvC = 0;
}

void time() {
  printTimeInterval = !printTimeInterval;  // toggle the flag
  if (printTimeInterval) {
    send(F("\r\n>>Started - Time printing"));
    lastTimePrint = millis();  // reset timer
  } else {
    send(F("\r\n>>Stopped - Time printing"));
  }
}

void game() {
  gameOn = 1;
  randomSeed(analogRead(0));
  initializeGrid();
  send(F("\r\nUse 'l' and 'r' to move left/right. Avoid falling O's!"));
  // printGrid();
}

void help() {
  char c;
  char *str = (char *)TXT_Help;
  while ((c = pgm_read_byte(str++)) != 0) {
    send(c);
  }
}
