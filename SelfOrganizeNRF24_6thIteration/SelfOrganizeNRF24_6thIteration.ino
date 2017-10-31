//6th update, adding leave without notifying, now also when packet goes to the left.

#include <SPI.h>
#include <RF24.h>
#include <string.h>
#include <printf.h>

//Type for packets to be sent and received
struct packetType{
  char type[8];
  //addresses at nRF24L01 have 40bits, so we make a vector of 5 8-bit indexes
  uint8_t senderNode[5];
  uint8_t receiverNode[5];
  //data is made to be a 8-index vector, just for testing purposes
  int dataSize;
  int data[8];
  int dataSizeEnd;  //dataSizeEnd = dataSize
};


//Initialize transceiver
RF24 radio(7,8);


//Global variables used

//Addresses:
//40bits addresses, only the first 8 bits really matter, so the [0] of the vectors.
//the rest is balanced 0s and 1s for sending to be successful.
uint8_t selfad[5] = {0x01, 0x0F, 0x0F, 0x0F, 0x0F};//self address
uint8_t broadcastad[5] = {0xFE, 0x0F, 0x0F, 0x0F, 0x0F};//address for broadcasts
uint8_t sendtoad[5] = {0xFF, 0x0F, 0x0F, 0x0F, 0x0F};//address for where to send the packet


int inNW = 0;//if it's =1, then the node is in the network.
char serialRead;//used to keep the value from reading from the serial (keyboard).
int NWcounter = 0; //how many nodes there are
int printtemp = 0, printtemp1 = 0; //used so only printed once
unsigned long int lastRefreshedTime = 0; //used for time intervals for broadcasting invites
//general packet used to store and process received packets.


//Packets used:
//this is where the received packets will be stored
struct packetType packet = {"0000000", 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x0F,
0x0F, 0x0F, 0x0F, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//packet used to send invitations at broadcastad address.
struct packetType broadPacket = {"invite", 0x01, 0x0F, 0x0F, 0x0F, 0x0F, 0x01, 0x0F,
0x0F, 0x0F, 0x0F, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1};


//FUNCTIONS
//-------------------
//Used to send packet
int sendPacket(struct packetType sPacket){
  //printPacket(sPacket);//print packet for checking
  int a;//this is the value to be returned
  
  //check where to send the packet
  if(sPacket.receiverNode[0] > selfad[0]) sendtoad[0] = selfad[0] + 1;
  else if(sPacket.receiverNode[0] < selfad[0]) sendtoad[0] = selfad[0] - 1;
  else sendtoad[0] = 0xFF;

  //send packet to sendtoad
  if(sendtoad[0] != 0xFF){
    radio.stopListening();
    radio.openWritingPipe(sendtoad);
    a = radio.write(&sPacket, sizeof(sPacket));
    a = radio.write(&sPacket, sizeof(sPacket));
    //radio.printDetails();
    delay(5);
    radio.closeReadingPipe(0);
    radio.startListening();
  }
  else a = -1;//if reached destination, a=-1


  if(a==0){//if write failed, assume sendtoad node left
    Serial.print(sendtoad[0]);
    Serial.println(" failed");


    //if it was to be sent to the right
    if(sendtoad[0] > selfad[0]){
      int oldNWcounter = NWcounter; //keep NWcounter to know how many consecutive nodes left
      //temporarily change selfad
      selfad[0]++;
      //make leave packet and send it
      struct packetType leavepacket = {"leave", 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x0F,
      0x0F, 0x0F, 0x0F, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      leavepacket.receiverNode[0] = NWcounter;
      leavepacket.senderNode[0] = selfad[0];
      leavepacket.dataSize=1;
      sendPacket(leavepacket);
      //go back to original selfad, to update left nodes
      selfad[0]--;
      NWcounter--;
      //only update nodes if sent message isn't leave, otherwise it's
      //consecutive leaves, and the last one will handle it all.
      if(strcmp(sPacket.type , "leave") != 0) updateNodes(0x01);
      //match destination with new receiver address.
      sPacket.receiverNode[0] = sPacket.receiverNode[0] - (oldNWcounter-NWcounter);
      a = sendPacket(sPacket);
    }


    //if it was to be sent to the left
    else if(sendtoad[0] < selfad[0]){
      //make leave packet and send it.
      struct packetType leavepacket = {"leave", 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x0F,
      0x0F, 0x0F, 0x0F, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      leavepacket.receiverNode[0] = NWcounter;
      leavepacket.senderNode[0] = selfad[0];
      leavepacket.dataSize=1;
      sendPacket(leavepacket);
      //reduce self address by 1 to keep network correct.
      selfad[0]--;
      radio.stopListening();
      radio.openReadingPipe(1,selfad);
      radio.startListening();
      NWcounter--;
      updateNodes(0x01);//update the nodes to the left with the new NWcounter.
      sPacket.senderNode[0]--;//match destination with new receiver address.
      //if, after the network is fixed, the receiver node is higher than the
      //NWcounter, then set it to NWcounter.
      if(sPacket.receiverNode[0] > NWcounter) sPacket.receiverNode[0] = NWcounter;
      //if about to be sent packet is update packet, don't send it,
      //because above updateNodes gave the most recent and correct info.
      if(strcmp(sPacket.type , "update") != 0) a = sendPacket(sPacket);
    }
  }
  return a;
}
//-------------------------------


//Used to inform every node from the node who sends it
//to the node with address recNode about the new NWcounter.
int updateNodes(uint8_t recNode){
  //make update packet
  struct packetType updatePacket = {"update", 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x01, 0x0F,
  0x0F, 0x0F, 0x0F, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1};
  updatePacket.senderNode[0] = selfad[0];
  updatePacket.receiverNode[0] = recNode;
  updatePacket.data[0] = NWcounter;
  printPacket(updatePacket);
  //send it to recNode
  int a = sendPacket(updatePacket);
  return a;
}
//------------------------------


//Used to print packet's content.
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



//Main program starts here.
void setup() {
  //Begin serial connection through USB
  Serial.begin(9600);
  printf_begin();

  Serial.println("Type a letter to begin:");
  //wait for command (just a random key from the keyboard) to start
  while(!Serial.available());
  Serial.read();
  Serial.println("Turned ON. Listening for invitations for 5 sec...");
  radio.begin();
  //set power to low, since we are testing
  //we don't need any higher that tires the transceiver.
  radio.setPALevel(RF24_PA_LOW);
  //start listening for broadcasts
  radio.openReadingPipe(1, broadcastad);
  radio.startListening();
  unsigned long int miltimer = millis();

  
  while(millis()-miltimer<5000 && inNW==0){//listen for 5 seconds for broadcasts.
    if(radio.available()){
      delay(5);
      Serial.println("Heard broadcast. Trying to join...");
      radio.read(&packet, sizeof(packet));
      radio.read(&packet, sizeof(packet));

      //If it's an actual invitation packet, do all the procedures needed.
      //Has to be type "invite" to be an actual invitation.
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

  //If reached this line with inNW==0,
  //it means they didn't receive anything and 5 seconds passed
  //so they assume they are the 1st node in the network.
  if(inNW==0){
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



//When in here, the node is in the network
void loop() {

  //If last node, send invitation every 3 sec.
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


  //If a packet was just received, process it and forward it, if needed.
  if(packet.dataSize!=0)
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

    //case "leave" packet
    else if(strcmp(packet.type, "leave")==0)
    {
      sendPacket(packet);
      NWcounter--;
      selfad[0]--;
      radio.stopListening();
      radio.openReadingPipe(1,selfad);
      radio.startListening();
    }

    //case "transit" packet
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
    packet.dataSize=0;//Clear data. Just needed to set dataSize=0.
  }

  //If there is no packet or serial, print that you are waiting.
  else{
    if(printtemp1==0){
      Serial.print("Waiting: ");
      Serial.println(selfad[0]);
      printtemp1=1;
    }

    //If there is nothing to listen to, try to read from serial.
    if(!radio.available()){

      //If there is something in serial, print NWcounter and self address,
      //then proccess it if needed
      if(Serial.available()){
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
          //read packet data, make it and send it.
          Serial.println("MAKING TRANSIT PACKET");
          strcpy(packet.type, "transit");
          Serial.print("Selfad = ");
          Serial.println(selfad[0]);
          Serial.print("NWcounter = ");
          Serial.println(NWcounter);
          packet.senderNode[0] = selfad[0];
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

    //If heard something from the selfad pipe
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
