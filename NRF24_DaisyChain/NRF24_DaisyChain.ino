#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <string.h>
RF24 radio(7, 8); // CNS, CE
//int address[3] = {0x0000000F0F0F0F00, 0x0000000F0F0F0F01, 0x0000000F0F0F0F02};
int selfad = 0x0000000F0F0F0F01;
int sendtoad = 0x0000000F0F0F0F00;
int a=0;
struct packetType{
  char type[8];
  int senderNode;
  int receiverNode;
  int dataSize;
  int data[8];
  int dataSizeEnd;
};

struct packetType packet = {"00000000",0,0,0,0,0,0,0,0,0,0,0,0};

void setup() {
  //Dhmiourgia arxeiou, prepei na sbistei sta alla nodes.
  strcpy(packet.type, "transit");
  packet.senderNode = 0x0000000F0F0F0F01;
  packet.receiverNode = 0x0000000F0F0F0F03;
  packet.dataSize = 8;
  for(int i=0; i<8; i++) packet.data[i] = 5*i;
  packet.dataSizeEnd = 8;
  //mexri edw na sbistei sta alla
  //CRC na kanw
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(1,((uint64_t)selfad&0x00000000FF)|0x0F0F0F0F00);
  radio.openWritingPipe(((uint64_t)selfad&0x00000000FF)|0x0F0F0F0F00);
  radio.openWritingPipe(((uint64_t)(selfad+1)&0x00000000FF)|0x0F0F0F0F00);
  radio.openWritingPipe(((uint64_t)(selfad-1)&0x00000000FF)|0x0F0F0F0F00);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
}

void loop() {
  delay(1000);
  if(packet.dataSize!=0)//exw data
  {
    //pou kai an tha to steilw
    if(packet.receiverNode>selfad) sendtoad = selfad+1;
    else if(packet.receiverNode<selfad) sendtoad = selfad-1;
    else sendtoad = -1;

    //to stelnw
    if(sendtoad!=-1)
    {
      Serial.println("selfad, sendtoad, telikoad");
      Serial.print(selfad);
      Serial.print(sendtoad);
      Serial.println(packet.receiverNode);
      Serial.println(sendtoad);
      delay(5);
      radio.stopListening();
      radio.openWritingPipe(((uint64_t)sendtoad&0x00000000FF)|0x0F0F0F0F00);
      a = radio.write(&packet, sizeof(packet));
      Serial.print("Write success = ");
      Serial.println(a);
      //mhdenizw
      a=0;
      strcpy(packet.type, "0000000");
      packet.senderNode = 0;
      packet.receiverNode = 0;
      packet.dataSize = 0;
      for(int i=0; i<8; i++) packet.data[i] = 0;
      packet.dataSizeEnd = 0;
      delay(5);
      //radio.openWritingPipe(0x0F0F0F0F07);//na mh diabazei apo to sendtoad
      //radio.openReadingPipe(1, selfad);
      radio.startListening();
    }
    else
    {
      Serial.println("Telos. Antistrefw.");
      int temp = packet.receiverNode;
      packet.receiverNode = packet.senderNode;
      packet.senderNode = temp;
      //sendtoad = 0x5555555550LL;
    }
  }
  else
  {
    Serial.print("Waiting: ");
    Serial.println(selfad);
    while(!radio.available());
    Serial.println("Kati akousa");
    radio.read(&packet, sizeof(packet));
    Serial.println("received: selfad, telikoad:");
    Serial.print(selfad);
    Serial.println(packet.receiverNode);
  }
}
