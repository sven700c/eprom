// Teensy 2
const int programPin=0;
const int readPin=3;
const int enablePin=16;

const int shiftDATA=4;
const int shiftCLK=1;
const int shiftLATCH=2;

const unsigned long romSize=1024*1024;

char dataPins[8]={13,14,15,5,6,7,8,9};

byte inByte=0;
unsigned int secH=0,secL=0;

void setup() {
  pinMode(programPin,OUTPUT);
  pinMode(readPin,OUTPUT);
  pinMode(enablePin,OUTPUT);
  pinMode(shiftDATA,OUTPUT);
  pinMode(shiftCLK,OUTPUT);
  pinMode(shiftLATCH,OUTPUT);

  digitalWrite(programPin,LOW);
  digitalWrite(readPin,LOW);
  digitalWrite(enablePin,HIGH);
  Serial.begin(250000);
  delay(1000);
  programMode();
}


int index=0;
void loop() {
  if(Serial.available()){
    inByte=Serial.read();
    if(inByte==0x55){
      while(Serial.available()==0);
      inByte=Serial.read();
      switch(inByte){
        case 'w':
          programMode();
          while(Serial.available()<2);
          secH=Serial.read();
          secL=Serial.read();
          writeSector(secH,secL);
          break;
        case 'r':
          readMode();
          readROM();
          break;
      }
    }
  }
}


//low level functions, direct ccontact with hardware pins
void programMode(){
  //data as output
  for(int i=0;i<8;i++){
    pinMode(dataPins[i],OUTPUT);
  }
  digitalWrite(readPin,LOW);
  digitalWrite(programPin,HIGH);
}


void readMode(){
  //data as input
  for(int i=0;i<8;i++){
    pinMode(dataPins[i],INPUT);
  }
  digitalWrite(programPin,LOW);
  digitalWrite(readPin,LOW);

}


void setAddress(unsigned long Address){
    shiftOut(shiftDATA, shiftCLK, MSBFIRST, (Address >> 16));
    shiftOut(shiftDATA, shiftCLK, MSBFIRST, (Address >> 8));
    shiftOut(shiftDATA, shiftCLK, MSBFIRST, Address);

    digitalWrite(shiftLATCH, LOW);
    digitalWrite(shiftLATCH, HIGH);
    digitalWrite(shiftLATCH, LOW);
}


byte readByte(unsigned long adr){
    byte data;
    setAddress(adr);
    digitalWrite(enablePin,LOW);
    delayMicroseconds(1);
    for(int i=7;i>=0;i--){
        data=data<<1;
        data|=digitalRead(dataPins[i])&1;
    }
    digitalWrite(enablePin,HIGH);
    return data;
}


void setData(char Data){
  for(int i=0;i<8;i++){
      digitalWrite(dataPins[i],Data&(1<<i));
  }
}


void programByte(byte Data){
  setData(Data);
  //Vpp pulse
  delayMicroseconds(4);
  digitalWrite(enablePin,LOW);
  delayMicroseconds(60);
  digitalWrite(enablePin,HIGH);
}


void writeSector(unsigned char sectorH,unsigned char sectorL){
  byte dataBuffer[128];
  unsigned long address=0;
  byte CHK=sectorH,CHKreceived;
  CHK^=sectorL;

  address=sectorH;
  address=(address<<8)|sectorL;
  address*=128;

  for(int i=0;i<128;i++){
        while(Serial.available()==0);
        dataBuffer[i]=Serial.read();
        CHK ^= dataBuffer[i];
  }
  while(Serial.available()==0);
  CHKreceived=Serial.read();
  programMode();
  //only program the bytes if the checksum is equal to the one received
  if(CHKreceived==CHK){
    for (int i = 0; i < 128; i++){
      setAddress(address++);
      programByte(dataBuffer[i]);
    }
  Serial.write(CHK);
  }
  readMode();
}


int readROM(){
  unsigned long num=1024*1024;
  unsigned long address;
  byte data,checksum=0;
  address=0;
  //read mode
  readMode();
  //start frame
  digitalWrite(readPin,LOW);
  digitalWrite(programPin,LOW);
  for(long i;i<1048576;i++){//1048576
    data=readByte(address++);
    Serial.write(data);
    //checksum^=data;
  }
  digitalWrite(readPin,HIGH);

  //Serial.write(checksum);
  //Serial.write(0xAA);
}
