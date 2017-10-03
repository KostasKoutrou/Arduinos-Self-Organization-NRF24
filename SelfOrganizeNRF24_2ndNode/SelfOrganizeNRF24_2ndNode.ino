#include <SPI.h>
#include <RF24.h>
#include <string.h>
#include <printf.h>

struct packetType{
  char type[8];
  uint8_t senderNode[5];
  uint8_t receiverNode[5];
  int dataSize;
  int data[8];
  int dataSizeEnd;
};

RF24 radio(7,8);

//Everyone start with self address = 1
uint8_t selfad[5] = {0x01, 0x0F, 0x0F, 0x0F, 0x0F};
uint8_t broadcastad[5] = {0xFE, 0x0F, 0x0F, 0x0F, 0x0F};
uint8_t sendtoad[5] = {0xFF, 0x0F, 0x0F, 0x0F, 0x0F};
int inNW = 0; //=1 when connected with the network
int NWcounter = 0; //how many nodes there are
int printtemp = 0, printtemp1 = 0; //used so only printed once
unsigned long int lastRefreshedTime = 0; //used for time intervals for broadcasting invites
struct packetType packet;
struct packetType broadPacket = {"invite", 0x01, 0x0F, 0x0F, 0x0F, 0x0F, 0x01, 0x0F,
0x0F, 0x0F, 0x0F, 1, 1, 1};


//-----------------------------
//Used to send packet
int sendPacket(struct packetType sPacket){
  int a;
  if(sPacket.receiverNode[0] > selfad[0]) sendtoad[0] = selfad[0] + 1;
  else if(sPacket.receiverNode[0] < selfad[0]) sendtoad[0] = selfad[0] - 1;
  else sendtoad[0] = 0xFF;

  //send packet to sendtoad
  if(sendtoad[0] != 0xFF){
    radio.stopListening();
    radio.openWritingPipe(sendtoad);
    a = radio.write(&sPacket, sizeof(sPacket));
    delay(5);
    radio.closeReadingPipe(0);
    radio.startListening();
  }
  else a = -1;//if reached destination, a=-1
  return a;
}
//-------------------------------
//Used to inform every node about the new NWcounter
int updateNodes(){
  //1st make update packet
  struct packetType updatePacket = {"update", 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x01, 0x0F,
  0x0F, 0x0F, 0x0F, 1, 1, 1};
  updatePacket.senderNode[0] = selfad[0];
  updatePacket.receiverNode[0] = 1;
  sendPacket(updatePacket);
  updatePacket.receiverNode[0] = NWcounter;
  sendPacket(updatePacket);
}
//------------------------------


void setup() {
  Serial.begin(19200);
  printf_begin();
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  //start listening for broadcasts
  radio.openReadingPipe(1, selfad);//So that pipe 2 has the whole correct address
  radio.closeReadingPipe(1);
  radio.openReadingPipe(2, broadcastad);
  radio.startListening();
}

void loop() {
  if(inNW==0)
  {
    if(printtemp == 0)
    {
      Serial.println("Turned ON. Listening for invitations for 5 sec...");
      printtemp = 1;
    }
    if(millis()>5000)
    {
      //1st node to be turned on in NW.
      Serial.println("1st node in NW, initializing...");
      printtemp = 0;
      selfad[0] = 0x01;
      radio.stopListening();
      radio.openReadingPipe(1,selfad);
      radio.closeReadingPipe(2);
      radio.startListening();
      lastRefreshedTime = millis();
      inNW = 1;
      NWcounter = 1;
    }
    //listen for broadcasts for 5sec.
    else
    {
      if(radio.available())
      {
        Serial.println("Heard broadcast. Trying to join...");
        radio.read(&packet, sizeof(packet));
        //check if it's an actual invitation
        Serial.println(packet.type);
        
        Serial.println(strcmp(packet.type, "invite"));
        if(strcmp(packet.type, "invite")==0)
        {
          selfad[0] = packet.senderNode[0] + 1;
          //Inform every node for new node in NW.
          NWcounter = selfad[0];
          Serial.println("Joined. Updating info on all nodes.");
          radio.stopListening();
          radio.closeReadingPipe(2);
          radio.startListening();
          updateNodes();
          printtemp = 0;
          lastRefreshedTime = millis();
          inNW = 1;
        }
      }
    }
  }
  else//inNW=1
  {
    if(printtemp==0)
    {
      radio.printDetails();
      printtemp=1;
      Serial.print("In the network: ");
      Serial.println(selfad[0]);
    }
    //Last node broadcasts invitations every 3sec.
    if(millis()-lastRefreshedTime > 3000 && selfad[0] == NWcounter)
    {
      //make invite packet
      Serial.println(broadPacket.type);
      broadPacket.senderNode[0] = selfad[0];
      //broadcast it
      radio.stopListening();
      //radio.setAutoAck(0);
      radio.openWritingPipe(broadcastad);
      radio.write(&broadPacket, sizeof(broadPacket));
      delay(5);
      radio.closeReadingPipe(0);
      //radio.setAutoAck(1);
      radio.startListening();
      lastRefreshedTime = millis();
    }
    if(packet.dataSize!=0)//have data
    {
      //case "update" packet
      if(strcmp(packet.type, "update")==0)
      {
        NWcounter = packet.senderNode[0];
        Serial.println(packet.type);
        Serial.print("No. of nodes = ");
        Serial.println(NWcounter);
        int a = sendPacket(packet);
        Serial.print("Send success: ");
        Serial.println(a);
        //clearPacket(packet);
        packet.dataSize=0;
      }
    }
    else//don't have data
    {
      if(printtemp1==0){
        Serial.print("Waiting: ");
        Serial.println(selfad[0]);
        printtemp1=1;
      }
      if(!radio.available()){
        if(Serial.available()){
          Serial.read();
          printtemp1=0;
          Serial.print("NWcounter = ");
          Serial.println(NWcounter);
        }
      }
      else{
        printtemp1=0;
        Serial.println("Heard something.");
        radio.read(&packet, sizeof(packet));
        Serial.println(packet.type);
      }
    }
  }
}
























