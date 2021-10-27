# UNIVERSAL_CNC
<p>main.cpp is the initial version of the code</p>
<p>serial_com.cpp is the oop version of the code which includes the following functions and can be used using the included example in the read me</p>
<p>this code was created for serial communication between CNC parts  or for i2c communication between diffrent machines </p>
<p>it starts by </p>
<p>the initializing function : comm.start_ser_com(SDA_PIN,SCL_PIN,fr,q_ln); fr refers to the serial communication baud rate , q_ln refers to the length of the que</p>
<p>and then the serial function ser_to_que(); get information from serial and put it into a que with length = q_ln</p>
<p>que_to_i2c(); pulls the information from the que after reading the slaves and serial printing there feedback and then writes it to the slaves</p> 


<p>example : </p>

<p>  #include <serial_com.cpp </p>
<p>  #define SDA_PIN 21</p>
<p>  #define SCL_PIN 22</p>
<p>  #define fr 9600</p>

<p>  void setup()</p>
<p>  {</p> 
<p>    comm.start_ser_com(SDA_PIN,SCL_PIN,fr,8);//initialise the i2c </p>
<p>  }</p>
<p>  void loop() </p>
<p>  {</p>
<p>  ser_to_que();//get information from serial and put it into a que </p>
<p>  que_to_i2c();//it only takes from the que if the slaves responds back with feedback , and then this feedback is serial printed </p>
<p>  }   </p>
  
  
  
  
 <p> other functions created in serial_com.cpp for our algorithm : </p>
 <p> bool que_request(int sl_addr,uint8_t* rec_by,int sl_ln);//it adds to the que the data (slave address , recived_bytes,slave length)</p>
 <p> void add_polled_slave(int slave_address);//moves the que</p>
 <p> void i2c_send(int s_addr , uint8_t* in_put,size_t length); it creats a packet from input bytes and then transfere it to slave by using Wire.write()</p>
 <p> bool read_i2c(int addr ,int max_respponse_len); reads from the slave address ( addr ) </p>
  
