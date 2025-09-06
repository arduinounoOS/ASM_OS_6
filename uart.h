#define COMMAND_BUFFER_SIZE 50
volatile char command_buffer[COMMAND_BUFFER_SIZE];
volatile uint8_t command_index = 0;
volatile bool SS_RecvC = false;
char buffer[15];

ISR(USART_RX_vect) {
  char received_char = UDR0;
  // send(received_char);
  // if (command_index < COMMAND_BUFFER_SIZE - 1) {
  send(received_char);
  if ((received_char == '\r') || (received_char == '\n')) {
    command_buffer[command_index++] = '\0';  // null-terminate
    SS_RecvC = true;                         // signal main loop
    command_index = 0;                       // reset for next command
  } else {
    command_buffer[command_index++] = received_char;
  }
  // } else {
  //   // buffer overflow, reset
  //   command_index = 0;
  // }
}

void uart_init(uint32_t baud_rate) {
  uint16_t ubrr = (F_CPU + (baud_rate * 8)) / (16 * baud_rate) - 1;
  UBRR0H = (ubrr >> 8);
  UBRR0L = (ubrr & 0xFF);
  UCSR0A = 0;                                            // default
  UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);  // TX, RX, RX int
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);                // 8N1
}

void send(char c) {
  while (!(UCSR0A & (1 << UDRE0)))
    ;
  UDR0 = c;
}
void sendl() {
  send("\r\n");
}
void send(const char *str) {
  while (*str) {
    send(*str++);
  }
}

void send(const __FlashStringHelper *str) {
  const char PROGMEM *p = (const char PROGMEM *)str;
  char c;
  while ((c = pgm_read_byte(p++))) {
    send(c);
  }
}

void send(int value) {
  itoa(value, buffer, 10);
  send((const char *)buffer);
}

void send(float value, int decimalPlaces) {
  dtostrf(value, 0, decimalPlaces, buffer);
  send((const char *)buffer);
}

void send(uint16_t value) {
  itoa(value, buffer, 10);  // Convert to decimal string
  send((const char *)buffer);
}

void send(uint32_t value) {
  ultoa(value, buffer, 10);  // Convert to decimal string
  send((const char *)buffer);
}

void sendh(uint8_t value) {
  const char hex_chars[] = "0123456789ABCDEF";
  char buf[3];
  buf[0] = hex_chars[(value >> 4) & 0x0F];
  buf[1] = hex_chars[value & 0x0F];
  buf[2] = '\0';
  send(buf);
}

void sflush() {
  // If the transmit buffer is already empty, no need to wait
  if (UCSR0A & (1 << UDRE0)) {
    // Nothing to send, return immediately
    return;
  }
  // Wait until the transmit buffer is empty (if it's not already)
  while (!(UCSR0A & (1 << UDRE0))) {
    // Wait for buffer to become empty
  }
  // Wait until the transmission is complete
  while (!(UCSR0A & (1 << TXC0))) {
    // Wait for the last byte to finish transmitting
  }
  // Clear the transmit complete flag
  UCSR0A |= (1 << TXC0);
}