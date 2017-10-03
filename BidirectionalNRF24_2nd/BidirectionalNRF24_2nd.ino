#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7, 8); // CNS, CE
const byte address[][6] = {"00001","00002"};
int number = '0';
void setup() {
  Serial.begin(19200);
  radio.begin();
  radio.openWritingPipe(address[1]);
  radio.openReadingPipe(1,address[0]);
  radio.setPALevel(RF24_PA_MIN);
}

void loop() {
  delay(2000);
  delay(5);
  radio.stopListening();
  number=(number+1)%10;
  radio.write(&number, sizeof(number));
  Serial.print("2nd, I write: ");
  Serial.println(number);
  delay(5);
  radio.startListening();
  while(!radio.available());
  radio.read(&number, sizeof(number));
  Serial.print("2nd, I read: ");
  Serial.println(number);
}
