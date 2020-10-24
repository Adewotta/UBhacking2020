volatile uint32_t& ARM_CYCLE_COUNT = *((uint32_t*)0xE0001004);
constexpr int SIXTY_FOUR_MICROSECONDS = 15360;

/*
 * Interrupt Service Routine for reading data
 * When the voltage falls, it indicated that a message is being sent
 * So we break out of whatever we are doing, and go to the service routine to deal with it
 * We read the Bytes being sent, we gather the requested data, and then we send the data back out
 */
void dataISR();

/*
 * A Low-Bit is 3 microsecond low followed by a 1 microsecond high
 * A High-Bit is 1 microsecond low followed by  a 3 microsecond high
 * A Stop-Bit is the same as a High-Bit
 * Documentation for what the bytes mean can be found here https://simplecontrollers.bigcartel.com/gamecube-protocol
 */
uint32_t readData();

void setup() {
  Serial.begin(3600);
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), dataISR, FALLING);
}

void loop() {

}

void dataISR() {
  noInterrupts();
  //READ DATA
  //GET VALUES
  //SEND DATA
  interrupts();
  return;
}

uint32_t readData() {
  ARM_CYCLE_COUNT = 0;
  uint32_t ReadingBuffer = 0;
  uint32_t BitsRead = 0;
  //No read should take longer than 64 microseconds, so if we go past this we should just exit the loop
  while (ARM_CYCLE_COUNT < SIXTY_FOUR_MICROSECONDS) {
    //While Low just waste cycles and wait
    while (digitalRead(2) == LOW) {
      //putting the ASM NOP prevent the loop from being optimized out or messed with by the compiler
      __asm__("NOP");
    }
    //Get Time the voltage was low, reset the cycles counter, and then increment the BitsRead counter
    uint32_t TimeSpentLow = ARM_CYCLE_COUNT;
    ARM_CYCLE_COUNT = 0;
    BitsRead += 1;
    //If less than 2 microseconds or 240 cycles, then add 1 to the reading buffer
    if (TimeSpentLow < 240) ReadingBuffer += 1;
    if (BitsRead % 8 == 0) {
      switch (ReadingBuffer) {
      case 0x00: //Console Probe
        return 0;
      case 0x41: //Origin Probe
        return 1;
      case 0x400300: //Poll request without rumble
        return 2;
      case 0x400301: //Poll request with rumble
        return 3;
      default:
        break;
      }
      //If we have not gotten a byte match, then shift the bit to the left once to make way for the next bit
      ReadingBuffer << 1;
    }
  }
  //It should never return 255 unless a reading error occured
  return 255;
}
