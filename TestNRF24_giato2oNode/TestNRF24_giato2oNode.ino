#include <SPI.h>
#include <RF24.h>
#include <string.h>
#include <printf.h>

RF24 radio(7,8);

uint8_t selfad[5] = {0x02, 0x0F, 0x0F, 0x0F, 0x0F};
uint8_t sendtoad[5] = {0x01, 0x0F, 0x0F, 0x0F, 0x0F};
int a=0;
uint8_t packet=0x00;
/*struct packetType{
  char type[8];
  uint8_t senderNode[5];
  uint8_t receiverNode[5];
  int dataSize;
  int data[8];
  int dataSizeEnd;
};

struct packetType packet = {"00000000",0,0,0,0,0,0,0,0,0,0,0,0};
*/
void setup() {
  //Dhmiourgia arxeiou, prepei na sbistei sta alla
  packet=3;
  //Mexri edw na sbistei sta alla
  //CRC
  Serial.begin(19200);
  printf_begin();
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1, selfad);
  //radio.openWritingPipe(selfad);
  radio.startListening();
}



void loop() {
  delay(5000);
  //if(packet.dataSize!=0)//exw data

  if(sendtoad[0] == 0x01) sendtoad[0]=0x03;
  else sendtoad[0]=0x01;
    delay(10);
    radio.stopListening();
    radio.openWritingPipe(sendtoad);
    //radio.printDetails();
    a = radio.write(&packet, sizeof(packet));
    delay(10);
    //radio.openWritingPipe(selfad);
    radio.startListening();
    Serial.print("Write success = ");
    Serial.println(a);
    
    //mhdenizw
    a=0;
}
