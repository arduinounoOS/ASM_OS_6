#define FS_MAX_FILES 6
#define FS_NAME_LEN 8
#define FS_MAX_FILE_SIZE 128
#define EE_DIR_ADDR (sizeof(uint16_t))
#define EE_DIR_WIYW_dtH (FS_NAME_LEN + sizeof(uint16_t))
#define EE_DATA_ADDR (EE_DIR_ADDR + FS_MAX_FILES * EE_DIR_WIYW_dtH)

int8_t FS_findSlot(const char *name) {
  char buf[FS_NAME_LEN + 1];
  for (int8_t slot = 0; slot < FS_MAX_FILES; slot++) {
    int da = EE_DIR_ADDR + slot * EE_DIR_WIYW_dtH;
    for (int i = 0; i < FS_NAME_LEN; i++) buf[i] = EEPROM.read(da + i);
    buf[FS_NAME_LEN] = 0;
    if (buf[0] && strcmp(buf, name) == 0) return slot;
  }
  return -1;
}

int8_t FS_findFree() {
  for (int8_t slot = 0; slot < FS_MAX_FILES; slot++) {
    int da = EE_DIR_ADDR + slot * EE_DIR_WIYW_dtH;
    uint16_t len;
    EEPROM.get(da + FS_NAME_LEN, len);
    if (len == 0) return slot;
  }
  return -1;
}

void FS_format() {
  // clear directory
  for (uint8_t slot = 0; slot < FS_MAX_FILES; slot++) {
    int da = EE_DIR_ADDR + slot * EE_DIR_WIYW_dtH;
    for (int i = 0; i < FS_NAME_LEN; i++) EEPROM.update(da + i, 0);
    uint16_t z = 0;
    EEPROM.put(da + FS_NAME_LEN, z);
  }
  send(F("\r\n>> FS Formatted."));
}

void FS_list() {
  uint8_t slots_left = 0;
  send(F("\r\n\nFile table:"));
  for (uint8_t slot = 0; slot < FS_MAX_FILES; slot++) {
    int da = EE_DIR_ADDR + slot * EE_DIR_WIYW_dtH;
    char name[FS_NAME_LEN + 1];
    for (int i = 0; i < FS_NAME_LEN; i++) name[i] = EEPROM.read(da + i);
    name[FS_NAME_LEN] = 0;  // null-terminate
    uint16_t len;
    EEPROM.get(da + FS_NAME_LEN, len);
    if (name[0]) {
      send("\r\n    ");
      send(name);
      int name_len = 0;
      while (name[name_len] != '\0' && name_len < 10) name_len++;
      for (int i = name_len; i < 10; i++) send(' ');
      char sizeStr[9];  // 8 digits + null
      itoa(len, sizeStr, 10);
      int sizeLen = 0;
      while (sizeStr[sizeLen] != '\0' && sizeLen < 8) sizeLen++;
      for (int i = sizeLen; i < 8; i++) send(' ');
      send(F("("));
      send(sizeStr);
      send(F(" bytes)"));
    } else {
      slots_left++;
    }
  }
  sendl();
  send(F("Slots left: "));
  send(slots_left);
  sendl();
}

void FS_read(const char *name) {
  int8_t slot = FS_findSlot(name);
  if (slot < 0) {
    send(F("\r\n>> Not found"));
    return;
  }
  int da = EE_DIR_ADDR + slot * EE_DIR_WIYW_dtH;
  uint16_t len;
  EEPROM.get(da + FS_NAME_LEN, len);
  int pa = EE_DATA_ADDR + slot * FS_MAX_FILE_SIZE;
  sendl();
  send(name);
  send(F(":\r\n"));
  send(F("#"));
  for (uint16_t i = 0; i < len; i++) {
    send((char)EEPROM.read(pa + i));
  }
  send(F("#"));
}

void FS_write(const char *name, const char *txt) {
  uint16_t len = strlen(txt);
  if (len > FS_MAX_FILE_SIZE) {
    send(F("\r\n>> Error: file too big"));
    return;
  }
  int8_t slot = FS_findSlot(name);
  if (slot < 0) slot = FS_findFree();
  if (slot < 0) {
    send(F("\r\n>> No free slot"));
    return;
  }
  int da = EE_DIR_ADDR + slot * EE_DIR_WIYW_dtH;
  for (int i = 0; i < FS_NAME_LEN; i++)
    EEPROM.update(da + i, i < (int)strlen(name) ? name[i] : 0);
  EEPROM.put(da + FS_NAME_LEN, len);

  int pa = EE_DATA_ADDR + slot * FS_MAX_FILE_SIZE;
  for (uint16_t i = 0; i < len; i++) EEPROM.update(pa + i, txt[i]);

  send(F("\r\n>> Wrote "));
  send(name);
  send(F(" ("));
  send(len);
  send(F(" bytes)"));
}

void FS_delete(const char *name) {
  int8_t slot = FS_findSlot(name);
  if (slot < 0) {
    send(F("\r\n>> Not found"));
    return;
  }
  int da = EE_DIR_ADDR + slot * EE_DIR_WIYW_dtH;
  for (int i = 0; i < FS_NAME_LEN; i++) EEPROM.update(da + i, 0);
  uint16_t z = 0;
  EEPROM.put(da + FS_NAME_LEN, z);
  send(F("\r\n>> Deleted "));
  send(name);
}

byte PRINT_INTERVAL = 30;

uint16_t FS_readBuf(const char *name, char *buf, uint16_t maxlen) {
  int8_t slot = FS_findSlot(name);
  if (slot < 0) return 0;
  // read length
  int da = EE_DIR_ADDR + slot * EE_DIR_WIYW_dtH;
  uint16_t len;
  EEPROM.get(da + FS_NAME_LEN, len);
  len = min(len, maxlen - 1);
  // read data
  int pa = EE_DATA_ADDR + slot * FS_MAX_FILE_SIZE;
  for (uint16_t i = 0; i < len; i++) buf[i] = EEPROM.read(pa + i);
  buf[len] = '\0';
  return len;
}

void loadPSInitFromFS() {
  char buffer[15];
  if (FS_readBuf("psinit", buffer, sizeof(buffer))) {
    PRINT_INTERVAL = atoi(buffer);
  } else {
    PRINT_INTERVAL = 60;
  }
  send(F("\r\n PRINT_INTERVAL: "));
  send(PRINT_INTERVAL);
}

void loadOSInit() {
  char buffer[15];
  if (FS_readBuf("osinit", buffer, sizeof(buffer))) {
    OS_TimeSlice = atoi(buffer);
  }
  OCR2A = OS_TimeSlice;
  send(F("\r\n OS_TimeSLice: "));
  send(OS_TimeSlice);
}