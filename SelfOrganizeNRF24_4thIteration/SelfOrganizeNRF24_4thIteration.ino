/* Communication between nodes. */
//testing github desktop potato

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


uint8_t selfad[5] = {0x01, 0x0F, 0x0F, 0x0F, 0x0F};
uint8_t broadcastad[5] = {0xFE, 0x0F, 0x0F, 0x0F, 0x0F};
uint8_t sendtoad[5] = {0xFF, 0x0F, 0x0F, 0x0F, 0x0F};
int inNW = 0;
char serialRead;
int NWcounter = 0; //how many nodes there are
int printtemp = 0, printtemp1 = 0; //used so only printed once
unsigned long int lastRefreshedTime = 0; //used for time intervals for broadcasting invites
struct packetType packet = {"0000000", 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x0F,
0x0F, 0x0F, 0x0F, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
struct packetType broadPacket = {"invite", 0x01, 0x0F, 0x0F, 0x0F, 0x0F, 0x01, 0x0F,
0x0F, 0x0F, 0x0F, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1};


//FUNCTIONS
//-------------------
int writePipe(struct packetType sPacket, uint8_t sendtoad[5]){
  int a;
  radio.stopListening();
  radio.openWritingPipe(sendtoad);
  a = radio.write(&sPacket, sizeof(sPacket));
  a = radio.write(&sPacket, sizeof(sPacket));
  //radio.printDetails();
  delay(10);
  radio.closeReadingPipe(0);
  radio.startListening();
  return a;
}
//-------------------
//Used to send packet
int sendPacket(struct packetType sPacket){
  int a;
  if(sPacket.receiverNode[0] > selfad[0]) sendtoad[0] = selfad[0] + 1;
  else if(sPacket.receiverNode[0] < selfad[0]) sendtoad[0] = selfad[0] - 1;
  else sendtoad[0] = 0xFF;

  //send packet to sendtoad
  if(sendtoad[0] != 0xFF){
    a = writePipe(sPacket, sendtoad);
    //handle write fails, suppose that if write failed => node left without saying it
    while(a==0){
      //immitate node with ad=sendtoad leaving
      //just send to next node for first iteration
      Serial.print(sendtoad[0]);
      Serial.println(" node failed. Sending to next, if it exists.");
      if(sendtoad[0]!=NWcounter && sendtoad[0]!=1){//if there is next node, send it, else do nothing.
        sendtoad[0] = sendtoad[0] + sendtoad[0] - selfad[0];
        a = writePipe(sPacket, sendtoad);
      }
      else a=-1;
    }
  }
  else a = -1;//if reached destination, a=-1
  return a;
}
//-------------------------------
//Used to inform every node about the new NWcounter
int updateNodes(uint8_t recNode){
  //1st make update packet
  struct packetType updatePacket = {"update", 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x01, 0x0F,
  0x0F, 0x0F, 0x0F, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1};
  updatePacket.senderNode[0] = selfad[0];
  updatePacket.receiverNode[0] = recNode;
  updatePacket.data[0] = NWcounter;
  printPacket(updatePacket);
  int a = sendPacket(updatePacket);
  return a;
}
//------------------------------
//Used to print packet.
void printPacket(struct packetType pPacket){
  Serial.println("PRINT PACKET:");
  Serial.print("Packet type: ");
  Serial.println(pPacket.type);
  Serial.print("Sender node: ");
  Serial.println(pPacket.senderNode[0]);
  Serial.print("Receiver node: ");
  Serial.println(pPacket.receiverNode[0]);
  Serial.print("Data size: ");
  Serial.println(pPacket.dataSize);
  Serial.print("Data: ");
  for(int i=0; i<pPacket.dataSize; i++){
    Serial.print(pPacket.data[i]);
    Serial.print(" ");
  }
  Serial.println("");
}
//-----------------------------


void setup() {
  Serial.begin(9600);
  printf_begin();

  Serial.println("Type a letter to begin:");
  while(!Serial.available());//wait for command to start
  Serial.read();
  Serial.println("Turned ON. Listening for invitations for 5 sec...");
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  //start listening for broadcasts
  radio.openReadingPipe(1, broadcastad);
  radio.startListening();
  unsigned long int miltimer = millis();
  while(millis()-miltimer<5000 && inNW==0){
    if(radio.available()){
      delay(5);
      Serial.println("Heard broadcast. Trying to join...");
      radio.read(&packet, sizeof(packet));
      radio.read(&packet, sizeof(packet));
      if(strcmp(packet.type, "invite")==0)
      {
        Serial.print("Sender node = ");
        Serial.println(packet.senderNode[0]);
        selfad[0] = packet.senderNode[0] + 1;
        Serial.print("Selfad = ");
        Serial.println(selfad[0]);
        //Inform every node for new node in NW.
        NWcounter = selfad[0];
        Serial.println("Joined. Updating info on all nodes.");
        radio.stopListening();
        radio.openReadingPipe(1,selfad);
        //radio.closeReadingPipe(2);
        radio.startListening();
        int b = updateNodes(0x01);
        packet.dataSize=0;
        Serial.print("Update sends success = ");
        Serial.println(b);
        lastRefreshedTime = millis();
        inNW = 1;
      }
    }
  }
  if(inNW==0){//1st node
    Serial.println("1st node in NW, initializing...");
    selfad[0] = 0x01;
    radio.stopListening();
    radio.closeReadingPipe(1);
    radio.openReadingPipe(1,selfad);
    radio.startListening();
    lastRefreshedTime = millis();
    inNW = 1;
    NWcounter = 1;
  }
  //radio.printDetails();
  Serial.print("In the network: ");
  Serial.println(selfad[0]);
}




void loop() {//in the network
  if(millis()-lastRefreshedTime > 3000 && selfad[0] == NWcounter)
  {
    //make invite packet
    broadPacket.senderNode[0] = selfad[0];
    Serial.println(broadPacket.type);
    Serial.print("Selfad = ");
    Serial.println(selfad[0]);
    //broadcast it
    radio.stopListening();
    //radio.setAutoAck(0);
    radio.openWritingPipe(broadcastad);
    radio.write(&broadPacket, sizeof(broadPacket));
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
      NWcounter = packet.data[0];
      Serial.println(packet.type);
      Serial.print("No. of nodes = ");
      Serial.println(NWcounter);
      int a = sendPacket(packet);
      Serial.print("Send success: ");
      Serial.println(a);
    }
    else if(strcmp(packet.type, "leave")==0)
    {
      NWcounter--;
      sendPacket(packet);
      selfad[0]--;
      radio.stopListening();
      radio.openReadingPipe(1,selfad);
      radio.startListening();
    }
    else if(strcmp(packet.type, "transit")==0)
    {
      if(sendPacket(packet) == -1){
        Serial.println("Transit reached destination.");
        printPacket(packet);
      }
      else{
        Serial.println("Transit forwarded.");
        printPacket(packet);
      }
    }
    packet.dataSize=0;
  }
  else{//don't have data
    if(printtemp1==0){
      Serial.print("Waiting: ");
      Serial.println(selfad[0]);
      printtemp1=1;
    }
    if(!radio.available()){
      if(Serial.available()){//If nothing to hear, read from serial
        serialRead = Serial.read();
        printtemp1=0;
        Serial.print("NWcounter = ");
        Serial.println(NWcounter);
        Serial.print("Selfad = ");
        Serial.println(selfad[0]);

        if(serialRead == 'L'){//case "leave"
          Serial.print("Leaving the network, selfad = ");
          Serial.println(selfad[0]);
          serialRead=0;
          //make leave packet and send to highest node
          strcpy(packet.type, "leave");
          packet.senderNode[0] = selfad[0];
          packet.receiverNode[0] = NWcounter;
          packet.dataSize=1;
          sendPacket(packet);
          printPacket(packet);
          NWcounter--;
          //update the lower nodes
          updateNodes(0x01);
          packet.dataSize=0;
          //turn off
          radio.powerDown();
          while(1);//stay here forever
        }
        else if(serialRead == 'T'){//case send "transit"
          Serial.println("MAKING TRANSIT PACKET");
          packet.senderNode[0] = selfad[0];
          strcpy(packet.type, "transit");
          Serial.print("Selfad = ");
          Serial.println(selfad[0]);
          Serial.print("NWcounter = ");
          Serial.println(NWcounter);
          Serial.print("Give Receiver Ad: ");
          while(!Serial.available());
          packet.receiverNode[0] = Serial.read() - '0';
          Serial.println(packet.receiverNode[0]);
          Serial.print("Give data size (up to 8): ");
          while(!Serial.available());
          packet.dataSize = Serial.read() - '0';
          Serial.println("Give data:");
          for(int i=0; i<packet.dataSize; i++){
            Serial.print(i);
            Serial.print(": ");
            while(!Serial.available());
            packet.data[i] = Serial.read() - '0';
            Serial.println(packet.data[i]);
          }
          Serial.println("TRANSIT PACKET MADE, INFO");
          printPacket(packet);
          sendPacket(packet);
          packet.dataSize = 0;
        }
        else if(serialRead == 'P'){//case "print details"
          radio.printDetails();
        }
      }
    }
    else{
      delay(5);
      printtemp1=0;
      Serial.println("Heard something");
      radio.read(&packet, sizeof(packet));
      radio.read(&packet, sizeof(packet));
      Serial.println(packet.type);
    }
  }
}
