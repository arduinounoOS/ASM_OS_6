#define A_Pin_Voltage A3

void VT_Initialize(void) {
  pinMode(A_Pin_Voltage, INPUT);
  //analogReference(INTERNAL);
}

float VT_ReturnVoltage(void) {
  return (analogRead(A_Pin_Voltage) * 0.0375 * 1.08);  // (5.00 / 1024) * ((10 + 1.50) / 1.50);
}

