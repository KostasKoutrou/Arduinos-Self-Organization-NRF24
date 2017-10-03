#include <SPI.h>
#include <RF24.h>
#include <string.h>
#include <printf.h>

RF24 radio(7,8);

uint8_t selfad[5] = {0x04, 0x0F, 0x0F, 0x0F, 0x0F};
uint8_t sendtoad[5] = {0x00, 0x0F, 0x0F, 0x0F, 0x0F};
int a=0;
int b=0;
int radread=0;
uint8_t packet=0x00;

void setup() {
  //CRC
  Serial.begin(57600);
  printf_begin();
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1, selfad);
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
    a = radio.write(&packet, sizeof(packet));
    //radio.printDetails();
    radio.startListening();
    //radio.printDetails();
    Serial.print("Write success = ");
    Serial.println(a);
    
    //mhdenizw
    a=0;
    packet=0x00;
    /*strcpy(packet.type, "0000000");
    packet.senderNode[0] = 0x00;
    packet.receiverNode[0] = 0x00;
    packet.dataSize = 0;
    for(int i=0; i<8; i++) packet.data[i] = 0;
    packet.dataSizeEnd = 0;*/
    }
    else//exw ftasei sto receiverNode
    {
      Serial.println("Eftasa");
      Serial.println(packet);
      a=0;
      packet=0x00;
      /*uint8_t temp = packet.receiverNode[0];
      packet.receiverNode[0] = packet.senderNode[0];
      packet.senderNode[0] = temp;*/
    }
  }
  else//den exw data
  {
    if(b==0){
    Serial.print("Waiting: ");
    Serial.println(selfad[0]);
    }
    b=1;
    //radio.printDetails();
    if(!radio.available()){
      if(Serial.available()){
        b=0;
        char cpacket[1];
        *cpacket = Serial.read();
        Serial.print("Read packet from serial: ");
        Serial.println(cpacket);
        packet = atoi(cpacket);
        Serial.println(packet);
      }
    }
    else{
      b=0;
      Serial.println("Kati akousa");
      radio.read(&packet, sizeof(packet));
      radio.read(&packet, sizeof(packet));
      Serial.println("received: selfad, telikoad:");
      Serial.print(selfad[0]);
      Serial.println(packet/*.receiverNode[0]*/);
    }
  }
}
