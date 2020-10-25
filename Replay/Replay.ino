#define ARM_CYCLE_COUNT (*(uint32_t*)0xE0001004)
constexpr int SIXTY_FOUR_MICROSECONDS = 15360;

uint8_t sendBuffer[10] = { 0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 0 };
/*
 * Interrupt Service Routine for reading data
 * When the voltage falls, it indicated that a message is being sent
 * So we break out of whatever we are doing, and go to the service routine to deal with it
 * We read the Bytes being sent, we gather the requested data, and then we send the data back out
 */
void dataISR();

/*
 * The Below applies to readData and sendData
 * A Low-Bit is 3 microsecond low followed by a 1 microsecond high
 * A High-Bit is 1 microsecond low followed by  a 3 microsecond high
 * A Stop-Bit is the same as a High-Bit
 * Documentation for what the bytes mean can be found here https://simplecontrollers.bigcartel.com/gamecube-protocol
 */
 int readData();
 //give a pointer to an array as well as how many bytes to send
 int sendData(uint8_t* sendingBuffer, uint8_t bytesToSend);

void setup() {
  Serial.begin(9600);
  pinMode(8, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(8), dataISR, FALLING);
}

void loop() {
}

void dataISR() {
  noInterrupts();
  
    int requestID = readData();
   //Wait a few microseconds until the stop bit finishes and the voltage settles
   for(int x = 0; x < 240; x++) {__asm__("NOP");}
   switch (requestID) {
    case 0: 
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;    
    }
  interrupts();
  Serial.printf("\n%x",requestID);
  return;
}

int readData() {
  ARM_CYCLE_COUNT = 0;
  int BitsRead = 0;
  uint32_t ReadingBuffer = 0;
  //We have to use some less clear syntax to read the value faster
  while (ARM_CYCLE_COUNT < SIXTY_FOUR_MICROSECONDS) {
    //Wait for the pin 8to drop in voltage
    while((REG_PORT_IN0 & PORT_PA21)>>21==1){__asm__("NOP");}
    uint32_t startCounter = ARM_CYCLE_COUNT;
    //While pin 8 is low
    while((REG_PORT_IN0 & PORT_PA21)>>21==0){__asm__("NOP");}
    
    if(ARM_CYCLE_COUNT-startCounter < 240)ReadingBuffer+=1;
    if (BitsRead++ % 8 == 0) {
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
    }
    ReadingBuffer<<=1;
  }
  //It should never return 255 unless a reading error occured
  return 0xfe;
}

int writeData() {
  ARM_CYCLE_COUNT = 0;
  int BitsRead = 0;
  uint32_t ReadingBuffer = 0;
  //We have to use some less clear syntax to read the value faster
  while (ARM_CYCLE_COUNT < SIXTY_FOUR_MICROSECONDS) {
    //Wait for the pin 8to drop in voltage
    while((REG_PORT_IN0 & PORT_PA21)>>21==1){__asm__("NOP");}
    uint32_t startCounter = ARM_CYCLE_COUNT;
    //While pin 8 is low
    while((REG_PORT_IN0 & PORT_PA21)>>21==0){__asm__("NOP");}
    
    if(ARM_CYCLE_COUNT-startCounter < 240)ReadingBuffer+=1;
    if (BitsRead++ % 8 == 0) {
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
    }
    ReadingBuffer<<=1;
  }
  //It should never return 255 unless a reading error occured
  return 0xfe;
}
