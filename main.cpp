#include <Arduino.h>
#include <Wire.h>
#include <WireSlaveRequest.h>
#define MAX_SLAVE_RESPONSE_LENGTH 32
int SDA_PIN = 21;
int SCL_PIN = 22;
int freq=9600;
// for time measurements:
constexpr int master_sending = 25;
constexpr int measure_process = 26;
constexpr int POLLING_PIN = 27;



/* ------------------------------------------------------- */
#if (0)
  inline void ev__rx_started(void)
  {
    digitalWrite(RX_LATENCY_PIN, HIGH);
  }

  inline void ev__msend(void)
  {
    digitalWrite(RX_LATENCY_PIN, LOW);
  }

  inline void ev__poll(void)
  {
    digitalWrite(POLLING_PIN, !digitalRead(POLLING_PIN));
    digitalWrite(TX_LATENCY_PIN, LOW);
  }

  inline void ev__smstart(void)
  {
    digitalWrite(TX_LATENCY_PIN, HIGH);
  }
#endif
/* --------------------------------------------------------- */

typedef enum PCRXStates_e
{
  RS_SLAVES_NUM,
  RS_SLAVE_LEN,
  RS_SLAVE_BYTES
} PCRXStates_t;




PCRXStates_t state = RS_SLAVES_NUM;
int slave_n;
int slave_number=1;
int slave_ln; // how many bytes we recive
int slave_addr;
int slave_rec_n;//hoe many we recived 
uint8_t rec_bytes [ MAX_SLAVE_RESPONSE_LENGTH ]; //recived bytes
uint8_t incomingByte = 0;
int current_read = 0;
WirePacker packer; 


void i2c_send_packed(int s_addr, WirePacker& wp);
#if 0
void i2c_send(int s_addr , uint8_t* in_put,size_t length);
#endif
bool read_i2c(int addr ,int max_respponse_len);


class Communication
{
  public:
  void start_ser_com(int SDA_PIN,int SCL_PIN,int freq,int q_ln)
  {
    
    Wire.begin(SDA_PIN, SCL_PIN);   // join i2c bus
    
  } 
  //other methods

  /// variables
};

Communication comm;


void setup()
{
  pinMode(master_sending, OUTPUT);
  pinMode(POLLING_PIN, OUTPUT);
  pinMode(measure_process, OUTPUT);
  Serial.begin(9600);                               // console with text
  Serial1.begin(9600, SERIAL_8N1, 33, 32);        // data
  Wire.begin(SDA_PIN, SCL_PIN, freq);   // join i2c bus
  delay(3000);
  Serial1.write("  initialised  ");
}

void loop()
{
  size_t n = Serial1.available();
  if (n > 0)
  {
    incomingByte = Serial1.read();
    switch(state)
    {
      case RS_SLAVES_NUM:
        slave_number=incomingByte;
        digitalWrite(master_sending,HIGH);
        //Serial.printf("Request for %d slaves:\n", slave_number);
        slave_n=slave_number;
        slave_addr=1;
        state = RS_SLAVE_LEN;                  //state wait for the length 
        break;

      case RS_SLAVE_LEN:
        //Serial.printf("Slave #%d: len %d.\n", slave_addr, incomingByte
        digitalWrite(POLLING_PIN,HIGH);
        if (incomingByte == 0)
        {

          slave_addr++;
          slave_n--;
          if (slave_n==0)
          {               //if current slave is 0 then go back to idle it should be inside the skip condition        
            state = RS_SLAVES_NUM;
            break;
          }
            // otherwise, we stay in RS_SLAVE_LEN
          break;
        }

        // len != 0:
        
        slave_ln = incomingByte; 
        state = RS_SLAVE_BYTES;                              // state of reciving the bytes
        slave_rec_n=0;                        //set the recived bytes count to zero
        
        packer.reset();
        break;

      case RS_SLAVE_BYTES:                                   //state recive bytes
        packer.write(incomingByte);    // after adding all data you want to send, close the packet
        
        slave_rec_n++; 
        digitalWrite(POLLING_PIN,LOW);
        //Serial.printf("%02x ", incomingByte);

        if (slave_rec_n >= slave_ln)
        {
          // finished with the current slave data:
          //Serial.printf("\n");
          packer.end();                   // now transmit the packed data
          i2c_send_packed(slave_addr, packer);
          slave_addr++;
          slave_n--;
          if (slave_n <= 0)
            { 
              // all done
              state = RS_SLAVES_NUM;
              digitalWrite(master_sending,LOW);
              break;
            }

          // otherwise - continue reading:
          state = RS_SLAVE_LEN;
          break;
        }
      break;
      default:;
    }
  }
  else if(state == RS_SLAVES_NUM)
  {                                  // if there is nothing sent and we are in the IDLE state read all the slaves
    if(current_read <= slave_number)
    {
      digitalWrite(POLLING_PIN,HIGH);
      read_i2c(current_read+1, 4);                 // read all the slaves 
      digitalWrite(POLLING_PIN,LOW);
      current_read++;                             // increase the number of slave to be read
    } else
    {
      current_read=0;                           // well in other states just make sure that there is no more slave to read 
    }
  }
}


void i2c_send_packed(int s_addr, WirePacker& wp)
{
  digitalWrite(measure_process,HIGH);
  // then add data the same way as you would with Wire
  Wire.beginTransmission(s_addr);

  printf("addr = %d.\n", s_addr);

  while (wp.available())
  {    // write every packet byte
    char b = wp.read();
    Serial.printf("%02x ", (int)b);
    Wire.write(b);
  }
  Wire.endTransmission();         // stop transmitting
  digitalWrite(measure_process,LOW);
}



#if 0
void i2c_send(int s_addr , uint8_t* in_put,size_t length){
      // first create a WirePacker that will assemble a packet
          WirePacker packer;               // then add data the same way as you would with Wire
          packer.write(in_put,length);    // after adding all data you want to send, close the packet
          packer.end();                   // now transmit the packed data
          Wire.beginTransmission(s_addr);
          while (packer.available()) {    // write every packet byte
             Wire.write(packer.read());
          }
          Wire.endTransmission();         // stop transmitting
  }
#endif

bool read_i2c(int addr ,int max_respponse_len)
{
  digitalWrite(master_sending,HIGH);
  uint8_t recived[max_respponse_len];
  size_t actual_respponse_len=0;
  // first create a WireSlaveRequest object
  // first argument is the Wire bus the slave is attached to (Wire or Wire1)

  WireSlaveRequest slaveReq(Wire,addr , max_respponse_len);

  //printf("reading from #%d.\n", addr);

  // optional:  attempts numper.
  slaveReq.setAttempts(1);

  // attempts to read a packet from an ESP32 slave.
  // there's no need to specify how many bytes are requested,
  // since data is expected to be packed with WirePacker,
  // and thus can have any length.
  bool success = slaveReq.request();
  
  if (success)
  {
    Serial.printf("Received from Slave:\n");

    while (slaveReq.available() > 0)
    {  // loop through all but the last byte
      //recived[actual_respponse_len] =(uint8_t) slaveReq.read();       // receive byte as a character
      char c = slaveReq.read();

      Serial.printf("%02x ", (int)c);
      Serial1.write(c);
    }
    Serial.printf("\n");
    digitalWrite(master_sending,LOW);
    return true;
  } else
  {
    return false;
  }
  return false;
}
