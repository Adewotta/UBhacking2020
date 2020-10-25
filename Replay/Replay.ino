#define ARM_CYCLE_COUNT (*(uint32_t*)0xE0001004)
constexpr int SIXTY_FOUR_MICROSECONDS = 15360;


uint8_t sendBuffer[10] = { 0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 0 };
uint8_t probeResponse[3] = { 0x09, 0x00, 0x03};
volatile bool polled = false;
/*
   Interrupt Service Routine for reading data
   When the voltage falls, it indicated that a message is being sent
   So we break out of whatever we are doing, and go to the service routine to deal with it
   We read the Bytes being sent, we gather the requested data, and then we send the data back out
*/
void dataISR();

/*
   Communication protocol
   A Low-Bit is 3 microsecond low followed by a 1 microsecond high
   A High-Bit is 1 microsecond low followed by  a 3 microsecond high
   A Stop-Bit is the same as a High-Bit
   Documentation for what the bytes mean can be found here https://simplecontrollers.bigcartel.com/gamecube-protocol
*/


/*
   Read data coming in from the console to the controller
*/
int readData();


/*
   Send a bit from the device to the console
   It uses the same communication standard as the one above
   120 cycles is 1 microsecond
*/
void sendBit(uint8_t output);


/*
   Send the data in an array to the console
*/
void sendData(uint8_t* sendingBuffer, uint8_t bytesToSend);

void setup() {
  Serial.begin(9600);
  pinMode(8, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(8), dataISR, FALLING);
}

void loop() {
  if (polled) {
    polled = false;
    Serial.write((uint8_t)0);
    Serial.readBytes((char*)sendBuffer, 8);
  }
}

void dataISR() {
  noInterrupts();

  int requestID = readData();
  //Wait a few microseconds until the stop bit finishes and the voltage settles
  for (int x = 0; x < 400; x++) {
    __asm__("NOP");
  }
  switch (requestID) {
    case 0:
      sendData(probeResponse, 3);
      polled = true;
      break;
    case 1:
      sendData(sendBuffer, 10);
      polled = true;
      break;
    case 2:
      sendData(sendBuffer, 8);
      polled = true;
      break;
    case 3:
      break;
  }
  interrupts();
  return;
}

int readData() {
  ARM_CYCLE_COUNT = 0;
  int BitsRead = 0;
  uint32_t ReadingBuffer = 0;
  //We have to use some less clear syntax to read the value faster
  while (ARM_CYCLE_COUNT < SIXTY_FOUR_MICROSECONDS) {
    //Wait for the pin 8to drop in voltage
    while ((REG_PORT_IN0 & PORT_PA21) >> 21 == 1) {
      __asm__("NOP");
    }
    uint32_t startCounter = ARM_CYCLE_COUNT;
    //While pin 8 is low
    while ((REG_PORT_IN0 & PORT_PA21) >> 21 == 0 || ARM_CYCLE_COUNT - startCounter < 120) {
      __asm__("NOP");
    }
    if (ARM_CYCLE_COUNT - startCounter < 300)ReadingBuffer += 1;
    BitsRead++;
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
    }
    ReadingBuffer <<= 1;
  }
  //It should never return 255 unless a reading error occured
  return ReadingBuffer;
}


void sendBit(uint8_t output) {
  //BEGIN BY SETTING THE PIN LOW
  ARM_CYCLE_COUNT = 0;
  if (output == 0) { //SEND A 0
    //SET LOW FOR 3 MICROSECONDS
    digitalWrite(8, LOW);
    while (ARM_CYCLE_COUNT < 360) {
      __asm__("NOP");
    }
    ARM_CYCLE_COUNT = 0;
    //SET HIGH FOR 1 MICROSECOND
    digitalWrite(8, HIGH);
    while (ARM_CYCLE_COUNT < 120) {
      __asm__("NOP");
    }
  }
  else if (output == 128) { //SEND A 1
    //SET LOW FOR 1 MICROSECOND
    digitalWrite(8, LOW);
    while (ARM_CYCLE_COUNT < 120) {
      __asm__("NOP");
    }
    ARM_CYCLE_COUNT = 0;
    //SET HIGH FOR 3 MICROSECONDS
    digitalWrite(8, HIGH);
    while (ARM_CYCLE_COUNT < 360) {
      __asm__("NOP");
    }
  }
  else {
    //SET LOW FOR 1 MICROSECOND
    digitalWrite(8, LOW);
    while (ARM_CYCLE_COUNT < 240) {
      __asm__("NOP");
    }
    ARM_CYCLE_COUNT = 0;
    //SET HIGH FOR 3 MICROSECONDS
    digitalWrite(8, HIGH);
    while (ARM_CYCLE_COUNT < 240) {
      __asm__("NOP");
    }

  }
}

void sendData(uint8_t* sendingBuffer, uint8_t bytesToSend) {
  detachInterrupt(digitalPinToInterrupt(8));
  pinMode(8, OUTPUT);
  //Read the frontmost bit of the byte, then shift the byte to the left by one, after 8 shifts move to the next byte;
  for (int bytes = 0; bytes < bytesToSend; bytes++) {
    uint8_t byteToSend = *sendingBuffer;
    for (int bits = 0; bits < 8; bits++) {
      sendBit(byteToSend & 0b10000000);
      byteToSend = byteToSend << 1;
    }
    sendingBuffer += 1;
  }
  //send stop bit
  sendBit(2);
  pinMode(8, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(8), dataISR, FALLING);
}
