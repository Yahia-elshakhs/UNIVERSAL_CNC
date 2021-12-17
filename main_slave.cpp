
#include <Arduino.h>
#include <Wire.h>
#include <WireSlave.h>

int SDA_PIN = 21;
int SCL_PIN = 22;
int sending = 25;
#define I2C_SLAVE_ADDR 0x01

void receiveEvent(int howMany);
void requestEvent();
uint8_t x[32] ;
size_t actual_recived=0;
unsigned long re_time;
volatile bool requested=false;

typedef enum state
{
  i2c_recive,
  process,
  i2c_send
} state_in;

state_in states = i2c_recive;


void setup()
{
    Serial.begin(9600);
    pinMode(sending,OUTPUT);
    bool success = WireSlave.begin(SDA_PIN, SCL_PIN, I2C_SLAVE_ADDR);
    if (!success) {
       // Serial.println("I2C slave init failed");
        while(1) delay(100);
    }

    WireSlave.onReceive(receiveEvent);
    WireSlave.onRequest(requestEvent);
    // Serial.printf("Slave joined I2C bus with addr #%d\n", I2C_SLAVE_ADDR);
    //
}

void loop()
{

    switch(states)
    {
      case i2c_recive:
        WireSlave.update();
        break;
      case process:
        delay(10);
        states = i2c_send;
        break;
      case i2c_send:
        WireSlave.update();
        break;
    }
      
    
}

// function that executes whenever a complete and valid packet
// is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  digitalWrite(sending,HIGH);
  // Serial.printf("Received %d:\n", howMany);

  //if(requested){return;}

  actual_recived=0;
  while (WireSlave.available() > 0) // loop through all but the last byte
  {
    char c = WireSlave.read();
    x[actual_recived] = (uint8_t)c;  // receive byte as uint8  
    //Serial.printf("%02recived  : ", (int)c);
    actual_recived++;
  
  }
states = process;
re_time = millis();
requested = true;
digitalWrite(sending,LOW);
//Serial.printf("\n");
}



void requestEvent()
{
  if (requested && millis()-re_time > 100 )
  {
    // printf("Sent back!\n");
    digitalWrite(sending,HIGH);
    WireSlave.write(x,actual_recived);
    digitalWrite(sending,LOW);
    //Serial.printf("%02recived  : ");
    //Serial.write(x,actual_recived);
    requested=false; 
    actual_recived = 0;
    states = i2c_recive;
  }
}



