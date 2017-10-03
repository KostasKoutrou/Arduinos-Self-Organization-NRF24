#include <SPI.h>
#include <RF24.h>
#include <string.h>
#include <printf.h>

RF24 radio(7,8);

uint8_t selfad[5] = {0x01, 0x0F, 0x0F, 0x0F, 0x0F};
uint8_t sendtoad[5] = {0x01, 0x0F, 0x0F, 0x0F, 0x0F};
int a=0;
uint8_t packet=0x00;

void setup() {
  //CRC
  Serial.begin(9600);
  printf_begin();
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1, selfad);
  //radio.openWritingPipe(selfad);
  radio.startListening();
}



void loop() {
  Serial.println("Waiting");
  while(!radio.available());
  //radio.printDetails();
  radio.read(&packet, sizeof(packet));
  Serial.println(packet);
}
