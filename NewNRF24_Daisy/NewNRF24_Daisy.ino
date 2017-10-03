#include <SPI.h>
#include <RF24.h>
#include <string.h>
#include <printf.h>

RF24 radio(7,8);

uint8_t selfad[5] = {0x01, 0x0F, 0x0F, 0x0F, 0x0F};
uint8_t sendtoad[5] = {0x00, 0x0F, 0x0F, 0x0F, 0x0F};
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
  /*strcpy(packet.type, "transit");
  for(int i=1; i<5; i++){
    packet.senderNode[i] = 0x0F;
    packet.receiverNode[i] = 0x0F;
  }
  packet.senderNode[0] = 0x01;
  packet.receiverNode[0] = 0x03;
  packet.dataSize = 8;
  for(int i=0; i<8; i++) packet.data[i] = 5*i;
  packet.dataSizeEnd = 8;*/
  packet=3;
  //Mexri edw na sbistei sta alla
  //CRC
  Serial.begin(9600);
  printf_begin();
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1, selfad);
  radio.openWritingPipe(selfad);
  radio.startListening();
}



void loop() {
  delay(100);
  //if(packet.dataSize!=0)//exw data
  if(packet!=0)
  {
    //pou kai an tha to steilw
    if(packet/*.receiverNode[0]*/>selfad[0]) sendtoad[0] = selfad[0] + 1;
    else if(packet/*.receiverNode[0]*/<selfad[0]) sendtoad[0] = selfad[0] - 1;
    else sendtoad[0]= 0xFF;
    
    //to stelnw
    if(sendtoad[0] != 0xFF)
    {
    Serial.println("selfad, sendtoad, telikoad");
    Serial.print(selfad[0]);
    Serial.print(sendtoad[0]);
    Serial.println(packet/*.receiverNode[0]*/);

    delay(10);
    radio.stopListening();
    radio.openWritingPipe(sendtoad);
    a = radio.write(&packet, sizeof(packet));
    delay(10);
    radio.openWritingPipe(selfad);
    radio.startListening();
    Serial.print("Write success = ");
    Serial.println(a);
    
    //mhdenizw
    a=0;
    packet=0;
    /*strcpy(packet.type, "0000000");
    packet.senderNode[0] = 0x00;
    packet.receiverNode[0] = 0x00;
    packet.dataSize = 0;
    for(int i=0; i<8; i++) packet.data[i] = 0;
    packet.dataSizeEnd = 0;*/
    }
    else//exw ftasei sto receiverNode
    {
      Serial.println("Telos. Antistrefw.");
      if(packet==3) packet=1;
      else packet = 3;
      /*uint8_t temp = packet.receiverNode[0];
      packet.receiverNode[0] = packet.senderNode[0];
      packet.senderNode[0] = temp;*/
    }
  }
  else//den exw data
  {
    Serial.print("Waiting: ");
    Serial.println(selfad[0]);
    radio.printDetails();
    while(!radio.available());
    Serial.println("Kati akousa");
    radio.read(&packet, sizeof(packet));
    Serial.println("received: selfad, telikoad:");
    Serial.print(selfad[0]);
    Serial.println(packet/*.receiverNode[0]*/);
  }
}
