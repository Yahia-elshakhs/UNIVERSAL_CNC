
  #include <serial_com.cpp> 
  #define SDA_PIN 21
  #define SCL_PIN 22
  #define fr 9600

  void setup()
  {
    //Communication comm; 
    comm.start_ser_com(SDA_PIN,SCL_PIN,fr,8);//initialise the i2c
  }
  void loop()
  {
  ser_to_que();//get information from serial and put it into a que 
  que_to_i2c();//it only takes from the que if the slaves responds back with feedback , and then this feedback is serial printed 
  }   




