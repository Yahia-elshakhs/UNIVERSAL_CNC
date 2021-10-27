#include <Arduino.h>
#include <Arduino.h>
#include <Wire.h>
#include <WireSlaveRequest.h>

#define SDA_PIN 21
#define SCL_PIN 22
int incomingByte = 0; // for incoming serial data
// set the max number of bytes the slave will send.
// if the slave send more bytes, they will still be read
// but the WireSlaveRequest will perform more reads
// until a whole packet is read
#define MAX_SLAVE_RESPONSE_LENGTH 32
#define MAX_Q_LN 8
#define MAX_POLLED 8


typedef struct Request_s{
bool ready;
int slave_addr;
uint8_t bytes[ MAX_SLAVE_RESPONSE_LENGTH ];
size_t sin_length ;//serial in length
}Request_t ;

typedef enum class PC2M_STATES : uint8_t{
IDLE,
W_S_LN, //WAIT FOR SLAVE LENGTH
RECIVE_BYTES,
}PC2M_State_t;

typedef enum class  SCHEDUELER_STATES : uint8_t{

}SCHEDUELER_State_t;

PC2M_State_t state = PC2M_STATES::IDLE;


Request_t requests[MAX_Q_LN];

size_t q_ind = 0 ;// reading the que
int slave_n;
int slave_ln; // how many bytes we recive
int slave_addr;
int slave_rec_n;//hoe many we recived 
uint8_t rec_bytes [ MAX_SLAVE_RESPONSE_LENGTH ]; //recived bytes

bool que_request(int sl_addr,uint8_t* rec_by,int sl_ln);
void add_polled_slave(int slave_add);
void i2c_send(int s_addr , uint8_t* in_put,size_t length);
bool read_i2c(int addr ,int max_respponse_len);
int polled_slave[MAX_Q_LN];
size_t polled_slave_count =0;
size_t polled_slave_current =0 ;

void setup()
{
    Serial.begin(9600);           // start serial for output
    Wire.begin(SDA_PIN, SCL_PIN);   // join i2c bus
    for(size_t i=0;i<MAX_Q_LN;i++){
      requests[i].ready=false;
    }
  pinMode(25,OUTPUT);

}

void loop()
{

// send data only when you receive data:
 if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();
    
    switch (state)
    {
    
    case PC2M_STATES::IDLE:
      if (incomingByte==0){
        break;
      }
      slave_n=incomingByte;
      slave_addr=1;
      state=PC2M_STATES::W_S_LN;
      digitalWrite(25,HIGH);
      //Serial.print(slave_n);

      break;
    case PC2M_STATES::W_S_LN:
      if (incomingByte==0)
      {
        slave_addr++;
        slave_n--;
        if (slave_n==0)
        {
          state=PC2M_STATES::IDLE;
          break;
        }
          
      break;
      }
      slave_ln=incomingByte;
      state=PC2M_STATES::RECIVE_BYTES;
      slave_rec_n=0;
      //Serial.print(slave_ln);
      break;
    case PC2M_STATES::RECIVE_BYTES:
      rec_bytes[slave_rec_n]=incomingByte;
      slave_rec_n++;

      if (slave_rec_n==slave_ln)
      {//save all of these bytes in a q
       if(! que_request(slave_addr,rec_bytes,slave_ln) ) {
         //Serial.print("error: could not que");
         //inform pc aboy full q
       };
       slave_addr++;
       slave_n--;
       if (slave_n==0)
        { digitalWrite(25,LOW);
          state=PC2M_STATES::IDLE;
          break;
        }
        state=PC2M_STATES::W_S_LN;
      }
      
      break;
    default:
      break;
    }
  }


if (polled_slave_count!=0){
int sl=polled_slave[polled_slave_current];
//Serial.print("polled :");
//erial.println(polled_slave_current);
read_i2c(sl,32);
polled_slave_current++;
if(polled_slave_current>=polled_slave_count) 
{polled_slave_current=0;}
}

//sending requests
if (requests[q_ind].ready){
uint8_t* p_data = requests[q_ind].bytes;
size_t data_length=requests[q_ind].sin_length;
int sl=requests[q_ind].slave_addr;
i2c_send(sl, p_data,data_length);
requests[q_ind].ready=false;
q_ind++;
if (q_ind>=MAX_Q_LN)
{
    q_ind=0;
}

}

}   


bool que_request(int sl_addr,uint8_t* rec_by,int sl_ln){
                            size_t i = q_ind;
                            size_t remaning = MAX_Q_LN;
                            for ( ; remaning>0 ; --remaning )
                            {
                              if (!requests[i].ready){
                                requests[i].slave_addr=sl_addr;
                                requests[i].sin_length=sl_ln;
                                memcpy(requests[i].bytes,rec_by,sl_ln);
                                requests[i].ready=true;
                                add_polled_slave(sl_addr);
                                return true;
                                
                              }
                            ++i;
                            if (i==MAX_Q_LN){
                                              i=0;
                                              }
                            
                             }
return false;
}





void add_polled_slave(int slave_add){
for (size_t i = 0; i < polled_slave_count; i++)
{
    if (polled_slave[i] == slave_add)
    {
        return;
    }
}

if (polled_slave_count==MAX_POLLED)
{
    //error report needed
    return;
}

polled_slave[polled_slave_count]=slave_add;
polled_slave_count++;

}



void i2c_send(int s_addr , uint8_t* in_put,size_t length){
    // first create a WirePacker that will assemble a packet
        WirePacker packer;

        // then add data the same way as you would with Wire
        packer.write(in_put,length);
        // after adding all data you want to send, close the packet
        packer.end();

        // now transmit the packed data
        Wire.beginTransmission(s_addr);
        while (packer.available()) {    // write every packet byte
            Wire.write(packer.read());
        }
        Wire.endTransmission();         // stop transmitting
}


bool read_i2c(int addr ,int max_respponse_len){
        uint8_t recived[max_respponse_len];
        size_t actual_respponse_len=0;
        // first create a WireSlaveRequest object
        // first argument is the Wire bus the slave is attached to (Wire or Wire1)
        WireSlaveRequest slaveReq(Wire,addr , max_respponse_len);

        // optional:  attempts numper.
        slaveReq.setAttempts(1);

        // attempts to read a packet from an ESP32 slave.
        // there's no need to specify how many bytes are requested,
        // since data is expected to be packed with WirePacker,
        // and thus can have any length.
        bool success = slaveReq.request();
        
        if (success) {
            while (slaveReq.available()>0) {  // loop through all but the last byte
                recived[actual_respponse_len] =(uint8_t) slaveReq.read();       // receive byte as a character
                actual_respponse_len++;
            }
            if (actual_respponse_len>0)
            {
              Serial.print("recived:  ");
              Serial.write(recived,actual_respponse_len);
              Serial.print("\n");
              return true;
            }
            
        }
        else {
            // if something went wrong, print the reason
        //we can add the readings of the rest here
        }
return false;
}
