/**** Uses arduino SP macro, PC is stored by ISR and Reti instructions. ****/
/**** Written by Kyle Davenport                                         ****/
/**** Use createtask() and startos()                                    ****/
/**** Stack sizes defined explicitly so compile size reflects stack memory */

#define OS_NumTasks 7

// Stack              Name              Max Seen
uint8_t stack0[130];  // shell           127
uint8_t stack1[45];   // Monitor          102
uint8_t stack2[55];   // Servo            96
uint8_t stack3[45];   // Motion          120
uint8_t stack4[95];   // Yaw             100 (without yaw calculations)
uint8_t stack5[45];   // Ultrasonic      50
uint8_t stack6[45];   // NP              45

volatile uint8_t OS_TimeSlice = 50;  // 130 is a bizarre sweet spot
#define monitorPeriod 10

volatile uint8_t timers[7];
volatile uint16_t isrCount = 0;  // count ISR fires per second
uint32_t OS_ISR_Clock = 0;
uint32_t OS_lastMs = 0;
uint32_t slice;

extern void send(char c);
extern void send(const char* str);
extern void send(const __FlashStringHelper* str);
extern void send(int);
extern void send(float, int);
extern void send(uint16_t);
extern void send(uint32_t);
extern void sendh(uint8_t);
extern void sendl();

typedef struct {
  volatile uint8_t* sp;
  void (*function)();
  uint8_t* stack;
  uint16_t stackSize;
  uint16_t highWaterMark;
  volatile uint32_t cpuAccum;  // run time in uS
  uint32_t lastStartUs;        // last start in uS
  volatile int minSliceUs;     // minimum single slice duration
  volatile int maxSliceUs;     // maximum single slice duration
} Task;

const char* const taskNames[7] = {
  "Shell",    //0
  "Monitor",  //1
  "NA",    //2
  "NA",   //3
  "NA",      //4
  "NA",   //5
  "NA"     //6
};

const uint8_t stackSizes[7] = { sizeof(stack0), sizeof(stack1), sizeof(stack2), sizeof(stack3), sizeof(stack4), sizeof(stack5), sizeof(stack6) };
uint8_t* stacks[7] = { stack0, stack1, stack2, stack3, stack4, stack5, stack6 };
volatile Task tasks[OS_NumTasks];
volatile uint8_t currentTask = 0;
uint8_t taskCount = 0;

void createTask(void (*function)(), uint8_t taskIndex) {
  if (taskIndex < OS_NumTasks && taskCount < OS_NumTasks) {
    tasks[taskIndex].stack = stacks[taskIndex];
    tasks[taskIndex].function = function;
    tasks[taskIndex].stackSize = stackSizes[taskIndex];
    tasks[taskIndex].sp = tasks[taskIndex].stack + tasks[taskIndex].stackSize - 1;
    tasks[taskIndex].highWaterMark = tasks[taskIndex].stackSize;              // Initialize high water mark
    memset((void*)tasks[taskIndex].stack, 0xAA, tasks[taskIndex].stackSize);  // Fill stack with known pattern
    uint16_t pc = (uint16_t)function;
    *(tasks[taskIndex].sp--) = (uint8_t)(pc & 0xFF);
    *(tasks[taskIndex].sp--) = (uint8_t)(pc >> 8);
    *(tasks[taskIndex].sp--) = 0x00;  // R0
    *(tasks[taskIndex].sp--) = 0x80;  // SREG
    for (int i = 1; i <= 31; i++) {
      *(tasks[taskIndex].sp--) = 0x00;
    }
    tasks[taskIndex].cpuAccum = 0;
    tasks[taskIndex].lastStartUs = 0;
    tasks[taskIndex].minSliceUs = 5000;  // Start with max value
    tasks[taskIndex].maxSliceUs = 0;     // Start with zero
    taskCount++;
  } else {
    send(F("\r\n  %%%%ERROR%     Task index out of bounds or maximum tasks reached\r\n"));
  }
}

// Data structure for captured slices
typedef struct {
  uint32_t slice;
  uint8_t task_id;
  uint32_t osclock;
  uint32_t last;
} MonitorData;

ISR(TIMER1_COMPA_vect, ISR_NAKED) {  // Hardware clears interrupt, asm call pushes next PC in last task
  asm volatile(
    "push r0 \n"
    "in r0, __SREG__ \n"
    "push r0 \n"
    "push r1 \n"
    "push r2 \n"
    "push r3 \n"
    "push r4 \n"
    "push r5 \n"
    "push r6 \n"
    "push r7 \n"
    "push r8 \n"
    "push r9 \n"
    "push r10 \n"
    "push r11 \n"
    "push r12 \n"
    "push r13 \n"
    "push r14 \n"
    "push r15 \n"
    "push r16 \n"
    "push r17 \n"
    "push r18 \n"
    "push r19 \n"
    "push r20 \n"
    "push r21 \n"
    "push r22 \n"
    "push r23 \n"
    "push r24 \n"
    "push r25 \n"
    "push r26 \n"
    "push r27 \n"
    "push r28 \n"
    "push r29 \n"
    "push r30 \n"
    "push r31 \n");

  tasks[currentTask].sp = (uint8_t*)SP;

  OS_ISR_Clock = micros();
  isrCount++;
  if (tasks[currentTask].lastStartUs != 0) {
    slice = OS_ISR_Clock - tasks[currentTask].lastStartUs;
    tasks[currentTask].cpuAccum += slice;
    if (slice < tasks[currentTask].minSliceUs) tasks[currentTask].minSliceUs = slice;
    if (slice > tasks[currentTask].maxSliceUs) tasks[currentTask].maxSliceUs = slice;
  }

  currentTask = (currentTask + 1) % taskCount;
  tasks[currentTask].lastStartUs = OS_ISR_Clock;
  SP = (uint16_t)tasks[currentTask].sp;

  asm volatile(
    "pop r31 \n"
    "pop r30 \n"
    "pop r29 \n"
    "pop r28 \n"
    "pop r27 \n"
    "pop r26 \n"
    "pop r25 \n"
    "pop r24 \n"
    "pop r23 \n"
    "pop r22 \n"
    "pop r21 \n"
    "pop r20 \n"
    "pop r19 \n"
    "pop r18 \n"
    "pop r17 \n"
    "pop r16 \n"
    "pop r15 \n"
    "pop r14 \n"
    "pop r13 \n"
    "pop r12 \n"
    "pop r11 \n"
    "pop r10 \n"
    "pop r9 \n"
    "pop r8 \n"
    "pop r7 \n"
    "pop r6 \n"
    "pop r5 \n"
    "pop r4 \n"
    "pop r3 \n"
    "pop r2 \n"
    "pop r1 \n"
    "pop r0 \n"
    "out __SREG__, r0 \n"
    "pop r0 \n"
    "reti \n");  // use op code reti to not adjust SREG or stack (vs C++ reti())
}

void __attribute__((naked)) yield() {  //naked does not store working registers, asm call pushes next PC of last function or task
  cli();
  asm volatile(
    "push r0 \n"
    "in r0, __SREG__ \n"
    "push r0 \n"
    "push r1 \n"
    "push r2 \n"
    "push r3 \n"
    "push r4 \n"
    "push r5 \n"
    "push r6 \n"
    "push r7 \n"
    "push r8 \n"
    "push r9 \n"
    "push r10 \n"
    "push r11 \n"
    "push r12 \n"
    "push r13 \n"
    "push r14 \n"
    "push r15 \n"
    "push r16 \n"
    "push r17 \n"
    "push r18 \n"
    "push r19 \n"
    "push r20 \n"
    "push r21 \n"
    "push r22 \n"
    "push r23 \n"
    "push r24 \n"
    "push r25 \n"
    "push r26 \n"
    "push r27 \n"
    "push r28 \n"
    "push r29 \n"
    "push r30 \n"
    "push r31 \n");

  tasks[currentTask].sp = (uint8_t*)SP;

  OS_ISR_Clock = micros();
  isrCount++;
  if (tasks[currentTask].lastStartUs != 0) {
    slice = OS_ISR_Clock - tasks[currentTask].lastStartUs;
    tasks[currentTask].cpuAccum += slice;
    if (slice < tasks[currentTask].minSliceUs) tasks[currentTask].minSliceUs = slice;
    if (slice > tasks[currentTask].maxSliceUs) tasks[currentTask].maxSliceUs = slice;
  }

  currentTask = (currentTask + 1) % taskCount;
  tasks[currentTask].lastStartUs = OS_ISR_Clock;
  SP = (uint16_t)tasks[currentTask].sp;

  asm volatile(
    "pop r31 \n"
    "pop r30 \n"
    "pop r29 \n"
    "pop r28 \n"
    "pop r27 \n"
    "pop r26 \n"
    "pop r25 \n"
    "pop r24 \n"
    "pop r23 \n"
    "pop r22 \n"
    "pop r21 \n"
    "pop r20 \n"
    "pop r19 \n"
    "pop r18 \n"
    "pop r17 \n"
    "pop r16 \n"
    "pop r15 \n"
    "pop r14 \n"
    "pop r13 \n"
    "pop r12 \n"
    "pop r11 \n"
    "pop r10 \n"
    "pop r9 \n"
    "pop r8 \n"
    "pop r7 \n"
    "pop r6 \n"
    "pop r5 \n"
    "pop r4 \n"
    "pop r3 \n"
    "pop r2 \n"
    "pop r1 \n"
    "pop r0 \n"
    "out __SREG__, r0 \n"
    "pop r0 \n"
    "reti \n");  // use op code reti to not adjust SREG or stack (vs C++ reti())
}

void StartOS() {
  SP = (uint16_t)tasks[currentTask].sp + 33;  // Set SP to first task and readjust SP for first task run (not reti from ISR)
  // TCCR2A = 0;                           // Clear control register A - run in normal mode
  // TCCR2B = 0;                           // Clear control register B
  // TCCR2A |= (1 << WGM21);               // Set CTC mode
  // TCCR2B |= (1 << CS21) | (1 << CS22);  // Prescaler 256
  // TIMSK2 |= (1 << OCIE2A);              // Enable Timer2 Compare Match A int             // 1ms at 16MHz/64 (250kHz, 4us per tick)
  // TCNT2 = 0;
  // OCR2A = OS_TimeSlice;  // Set compare match for ~1ms
  // Note: Timer setup sequence is important! The below sequence works:
  TCCR1A = 0;     //Timer setup must follow this sequence.
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);  // CTC mode
  TCCR1B |= (1 << CS12);   // 256
  TCNT1 = 0;
  OCR1A = OS_TimeSlice;     // For 20ms at prescaler 8, (16MHz / 8) = 2MHz, 0.5us per tick
  TIMSK1 |= (1 << OCIE1A);  // Enable compare match A interrupt
  reti();                   // Loads PC from new stack, sets interrupts - next clock cycle jumps to first task
}


void stackMonitorTask(void*) {  //48 uS loop
  for (;;) {
    // delay(500);
    // send("SM\n");
    for (uint8_t i = 0; i < taskCount; i++) {
      // find first non-0xAA entry from the bottom of the stack
      uint16_t used = 0;
      for (uint16_t j = 0; j < tasks[i].stackSize; j++) {
        if (tasks[i].stack[j] != 0xAA) {
          used = j;
          break;
        }
      }
      tasks[i].highWaterMark = tasks[i].stackSize - used;
      // yield();
    }
    uint32_t currentMillis = millis();
    if ((currentMillis - OS_lastMs) >= 1) {
      // Decrement timers if above zero
      for (uint8_t i = 0; i < 7; i++) {
        if (timers[i] > 0) {
          timers[i]--;
        }
      }
      OS_lastMs = currentMillis;  // Reset the timer reference
    }
  }
}