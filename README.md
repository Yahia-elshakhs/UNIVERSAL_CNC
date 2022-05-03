this Respirotry is the code for "Communication System for Standardized Multipurpose CNC Machine," 2021 IEEE 9th Workshop on Advances in Information, Electronic and Electrical Engineering (AIEEE), 2021, pp. 1-5, doi: 10.1109/AIEEE54188.2021.9670310.


The very first step for universal CNC project is receiving and sending the data from and to the computer using serial communication.

Doing so in the Arduino framework is pretty easy we initialize the serial by writing Serial.begin(freq) in the setup, Freq can be changed depending on our needs.

And then we write in the void loop.

1. if(Serial.available()\&gt;0)
2. {
3. IncomingByte=Serial.read();
4. // rest of the code
5. }

And then we need to process these information and send it to the I2C so we need to create a protocol to classify data received.

To do so we have agreed on the following protocol:

_Equation_ _1_ _serial array protocol_

Let us give an example:

[3, 2, x, y, 1, a, 3, f, g, h]

So, number of slave = 3, length of payload for first slave = 2, array to be sent to slave 1 [x, y], Length of 2nd slave payload = 1, array of slave 2 = [a], Length of 3rd slave payload = 3, array of slave 3 [f, g, h].

Note:

the slave addresses will be initialized manually on both the computer and the slave hardware also all the data sent will be in integer form the data will differ from a slave to slave but all slaves need to received data nearly at the same time, cause in case of CNC devices the command of the tool (laser for example) should be executed only when x, y is at the exact point needed so we cannot give order to the tool without making sure the x-y-escalators is already where they needed so we will need to send nothing to some slaves while sending data to other slaves and in this case it is pretty simple we just need to make the length of the slave payload = 0.

Example:

[8,1,x,1,y,1,z,0,0,0,0,0]

Here we have sent the x y z values while leaving out 5 other slaves we can also activate just one of these slaves like:

[8,1,x,1,y,1,z,0,0,1,data,0,0]

The slave number 6 received the value of data in this case while the other slaves received nothing

And so based on the soft protocol explained we are going to make code using the machine state algorithm.

After this state machine if the state is (receive number of slaves) then receive data from all of the slaves using I2C.

Note:

(I2C functions will be discussed after this state classification code)

**State machine code:**

1. if(Serial1.available() \&gt;0)
2. {
3. incomingByte =Serial1.read();
4. switch(state)
5. {
6. case RS\_SLAVES\_NUM: // idle state wait for slave number
7. slave\_number=incomingByte;
8. slave\_n=slave\_number;
9. slave\_addr=1;
10. state = RS\_SLAVE\_LEN;// state wait for the length
11. break;
12.
13. case RS\_SLAVE\_LEN:
14. digitalWrite(POLLING\_PIN,HIGH);
15. if(incomingByte ==0)
16. {
17. slave\_addr++;
18. slave\_n--;
19. if(slave\_n==0)
20. { //if current slave is 0 then go back to idle
21. state = RS\_SLAVES\_NUM;
22. break;
23. }
24. // otherwise, we stay in RS\_SLAVE\_LEN
25. break;
26. }
27. slave\_ln = incomingByte;
28. state = RS\_SLAVE\_BYTES;// state of receiving the bytes
29. slave\_rec\_n=0;//set the recived bytes count to zero
30. packer.reset();
31. break;
32.
33. case RS\_SLAVE\_BYTES://state receive bytes
34. packer.write(incomingByte);// add all data to packet
35. slave\_rec\_n++;
36. digitalWrite(POLLING\_PIN,LOW);
37. //Serial.printf(&quot;%02x &quot;, incomingByte);
38.
39. if(slave\_rec\_n \&gt;= slave\_ln)
40. {
41. // finished with the current slave data:
42. packer.end();// now transmit the packed data
43. I2C\_send\_packed(slave\_addr, packer);
44. slave\_addr++;
45. slave\_n--;
46. if(slave\_n \&lt;=0)
47. {
48. state = RS\_SLAVES\_NUM;
49. digitalWrite(RX\_LATENCY\_PIN,LOW);
50. break;
51. }
52.
53. // otherwise - continue reading:
54. state = RS\_SLAVE\_LEN;
55. break;
56. }
57.
58. default:;
59. }
60. }
61.
62.
63. if(state == RS\_SLAVES\_NUM)
64. {// if there is nothing sent and we are in the IDLE state read all the slaves
65. if(current\_read \&lt;= slave\_number)
66. {
67. digitalWrite(POLLING\_PIN,HIGH);
68. read\_I2C(current\_read,32);// read all the slaves
69. digitalWrite(POLLING\_PIN,LOW);
70. current\_read++;// increase the number of slave to be read
71. }else
72. {
73. current\_read=0; // in other states make sure that there is no more slave to read
74. }
75. }
76.

 ![Shape2](RackMultipart20220107-4-1w3se40_html_9bc1b403680131ba.gif)

_Figure 17 I2C packet format_

![](RackMultipart20220107-4-1w3se40_html_a67fd1303f6d74b3.png)The very basic of I2C control is the packaging of the data and error handling thankfully we found a great open source library [18] That handles these issues wonderfully using a function created called wire packer/unpacker which uses CRC-8 for error handling.

| Packet format: |
 |
| --- | --- |
|
 |
 |
|
 | \* [0]: start byte (0x02) |
|
 | \* [1]: packet length |
|
 | \* [2]: data[0] |
|
 | \* [3]: data[1] |
|
 | \* ... |
|
 | \* [n+1]: data[n-1] |
|
 | \* [n+2]: CRC8 of packet length and data |
|
 | \* [n+3]: end byte (0x04) |

And then we used [18] library to create 4 functions:

Master side 1st function is sending function while the 2nd function is the receiving function while slave side 3rd function receiving slave and 4th function sending from slave.

**1**** st **** function master sending function:**

1. // S\_addr represents the address of slave that will receive the data
2. // WirePacker is the packed array that we are going to send
3. Void I2C\_send\_packed(int s\_addr,WirePacker&amp; wirepacked)
4. {
5. Wire.beginTransmission(s\_addr);
6. while(wirepacked.available ())
7. {// write every packet byte
8. char bytes = wirepacked.read();
9. Wire.write(bytes);
10. }
11. Wire.endTransmission();// stop transmitting
12. digitalWrite(TX\_LATENCY\_PIN,LOW);
13. }

**2**** nd **** function master sending function:**

1. bool read\_I2C(int addr,int max\_respponse\_len)
2. {
3. uint8\_t recived[max\_respponse\_len];
4. size\_t actual\_respponse\_len=0;
5. // first create a WireSlaveRequest object
6. // first argument is the Wire bus the slave is attached to
7. // (Wire or Wire1)
8.
9. WireSlaveRequest slaveReq(Wire, addr , max\_respponse\_len );
10.
11.
12. // optional: attempts number.
13. slaveReq.setAttempts(1);
14.
15. // attempts to read a packet from an ESP32 slave.
16. // there&#39;s no need to specify how many bytes are requested,
17. // since data is expected to be packed with WirePacker,
18. // and thus can have any length.
19. bool success = slaveReq.request();
20.
21. if(success)
22. {
23. Serial.printf(&quot;Received from Slave: \n&quot;);
24. Serial.write(addr);
25.
26. while(slaveReq.available()\&gt;0)
27. {
28. // loop through all but the last byte
29. // receive byte as a character
30. char recived = slaveReq.read();
31.
32. Serial.printf(&quot;%02x &quot;,(int) recived);
33. Serial1.write(c);
34. }
35. Serial.printf(&quot;\n&quot;);
36. returntrue;
37. }else// if not successful
38. {
39. returnfalse;
40. }
41. }

**3**** rd **** function slave receiving function:**

1. void receiveEvent()
2. {
3. actual\_recived=0;
4. while(WireSlave.available()\&gt;0)// loop through all but the last byte
5. {
6. char c =WireSlave.read();
7. x[actual\_recived]=(uint8\_t) c;// receive byte as uint8
8. Serial.printf(&quot;%02x &quot;,(int) c);
9. actual\_recived++;
10. }
11. requested =true;
12.  }

  **4**** th **** function slave sending function:**

1. // since we still don&#39;t have data to send back we are going to send what we recived x is the array we recived, actual\_recived is the length of the array
2. Void requestEvent ()
3. {
4. If(requested)
5. {
6. Delay(100);
7. WireSlave.write(x, actual\_recived);
8. Serial.write(x, actual\_recived);
9. requested=false;
10. }
11. }

In our plan here will be a blocking operation in the slave so we need to create a state machine code that can allow the blocking operation without being interrupted by the interrupt function so the full slave code will be:

1. void receiveEvent(int howMany);
2. void requestEvent();
3. uint8\_t x[32];
4. size\_t actual\_recived=0;
5. unsignedlong re\_time;
6. volatilebool requested=false;
7.
8. typedefenum state
9. {
10.  I2C\_recive,
11.  process,
12.  I2C\_send
13. } state\_in;
14.
15. state\_in states = I2C\_recive;
16.
17. void setup()
18. {
19.   Serial.begin(9600);
20.   pinMode(sending,OUTPUT);
21.   bool success =WireSlave.begin(SDA\_PIN, SCL\_PIN, I2C\_SLAVE\_ADDR);
22.   if(!success){
23.     // Serial.println(&quot;I2C slave init failed&quot;);
24.     while(1) delay(100);
25.   }
26.
27.   WireSlave.onReceive(receiveEvent);
28.   WireSlave.onRequest(requestEvent);
29.   // Serial.printf(&quot;Slave joined I2C bus with addr #%d\n&quot;, I2C\_SLAVE\_ADDR);
30.   //
31. }
32.
33. void loop()
34. {
35.   if(actual\_recived ==0)
36.  {
37.   WireSlave.update();
38.  }else
39.  {
40.   if(m.drive\_ratio(recived[0]\*100+recived[1],recived[3]\*100+recived[3])) // this function will be
41. // discussed later
42.   {
43.    sent\_count=3;
44.    send[0]=1;
45.    send[1]=2;
46.    send[2]=3;
47.    actual\_recived =0;
48.   }
49.  }
50. }
