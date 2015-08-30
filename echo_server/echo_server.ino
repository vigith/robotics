#define DEBUG 1
#define PORT 8888

#define SSID     "--"
#define SSPASSWD "--"

char rn[] = "\r\n";
char ok[] = "OK\r\n";
char ready[] = "ready\r\n";

#define BUFFER_SIZE 512
/* data when processing loop */
char buffer[BUFFER_SIZE];

/* sends a command and test whether we got the ack string */
boolean checkSendDataAck(String command, char *ack, const int timeout) {
  boolean flag = false;              /* did we see the ack string */

  if (DEBUG)
    Serial.print(command);
  
  /* write to module */
  Serial1.print(command); // send the read character to the Serial1

    /* set timeout */
  Serial1.setTimeout(timeout);

  unsigned long time = millis();
  while (time + timeout > millis()) {
    if(Serial1.available()) {
      if(Serial1.find(ack)) {          
        flag = true;
        break;
      }
      if (flag)
        break;
    }
  }

  return flag;
}

boolean getIpAddress(const int timeout, String *ip) {
  boolean status = false;
  char pattern[] = "+CIFSR:STAIP,\"";
  String command = "";
    
  Serial1.setTimeout(timeout);

  command = "AT+CIFSR\r\n";
  Serial1.print(command);

  if (Serial1.find(pattern)) {
    status = true;
  }

  /* if status is true, it means we have seen the pattern */
  *ip = Serial1.readStringUntil('"');
 
  return status;
}


/* sends a command and spit out the response */
void checkSendDataDebug(String command, const int timeout) {
  if (DEBUG)
    Serial.print(command);
  
  /* write to module */
  Serial1.print(command); // send the read character to the esp8266

  unsigned long time = millis();
  while (time + timeout > millis()) {
    if(Serial1.available()) {
      char data = (char)Serial1.read();
      Serial.write(data);
    }
  }

  return;
}

/* clear the buffer in esp8266 */
boolean clearBuffer(const int timeout) {
  boolean flag = false;
  /* set timeout */
  Serial1.setTimeout(timeout);


  unsigned long time = millis();
  while (time + timeout > millis()) {
    if(Serial1.available()) {
      if(Serial1.find(rn)) {          
        flag = true;
        break;
      }
      if (flag)
        break;
    }
  }

  return flag;
}

boolean startServer(void) {
  String command = "";
  boolean status = true;
  short int retry;
    
  // AT (attention, just to make sure module is responding)
  command = "AT\r\n";
  status = false; retry = 4;
  while((status = checkSendDataAck(command, ok, 2000)) == false && retry-- > 0)
    if (DEBUG)
      Serial.println("retry");


  /* configure it as a client */
  command = "AT+CWMODE=1\r\n";
  while((status = checkSendDataAck(command, ok, 2000)) == false && retry-- > 0)
    if (DEBUG)
      Serial.println("retry");  

  command = "AT+CWJAP?\r\n";
  char ack[] = SSID;
  status = checkSendDataAck(command, ack, 2000);
  clearBuffer(1000);

  /* if not connected, try to connect */
  if (not status) {
    Serial.println("Not connected to WiFi.. Trying to Connect");
      command = "AT+CWJAP==\"";
      command += SSID;
      command += "\",\"";
      command += SSPASSWD;
      command += "\"\r\n";
      status = false; retry = 4;
      checkSendDataDebug(command, 5000);
      while((status = checkSendDataAck(command, ok, 5000)) == false && retry-- > 0)
        if (DEBUG)
          Serial.println("retry");
  }

  /* get ip from */
  String ip;
  status = getIpAddress(1000, &ip);


  /* allow it for single IP connection */
  command = "AT+CIPMUX=1\r\n";
  status = false; retry = 4;
  while((status = checkSendDataAck(command, ok, 2000)) == false && retry-- > 0)
    if (DEBUG)
      Serial.println("retry");

  /* start the server at given port (1 = create, PORT = port) */
  command = "AT+CIPSERVER=1,";
  command += PORT;
  command += "\r\n";
  status = false; retry = 4;
  while((status = checkSendDataAck(command, ok, 2000)) == false && retry-- > 0)
    if (DEBUG)
      Serial.println("retry");
  
  
  return status;
}


void setup() {
  /* boolean status for function return values  */
  boolean status = false;
  
  Serial.begin(9600);
  Serial1.begin(115200); // your esp's baud rate might be different (57600) 115200

  /* connect to wifi */
  status = startServer();
  if (DEBUG)
    Serial.println(status);

  if (status) {
      Serial.println("Started Server");        
  } else {
    while(1);
  }

}

/* read the EOL, ie \r\n */
bool readTillEol() {
  /* static is used because we might leave out of this function before getting all the chars, in that case
     we want to know the buffer postn and push more data from where we left off. */
  static int i=0;
  
  /* data is available */
  if(Serial1.available()) {    
    /* push to buffer */
    buffer[i++] = Serial1.read();
    
    /* overflow, reset to top */
    if(i == BUFFER_SIZE)
      i = 0;
    
    // 'i' should be atleast 2 (ie, > 1) to test for a successful \r\n
    // 13 = cr and 10 = nl
    if(i>1 && buffer[i-2]==13 && buffer[i-1]==10) {
      /* set to \0 */
      buffer[i]=0;
      /* set i = 0, we have read \r\n, next invocation of readTillEol is for a fresh buffer */
      i = 0;
      
      if (DEBUG)
        Serial.print(buffer);
      
      return true;
    }
  }
  
  return false;
}

void echo_back(int channel_id, int packet_len, char *buffer_ptr, const int timeout) {
  boolean flag = false;
  char send[] = "> ";
  Serial.println(channel_id);
  Serial.println(packet_len);
  Serial.println(buffer_ptr);
  
  Serial1.print("AT+CIPSEND=");
  Serial1.print(channel_id);
  Serial1.print(",");
  Serial1.print(packet_len);
  Serial1.print("\r\n");

  unsigned long time = millis();
  while (time + timeout > millis()) {
    if(Serial1.available()) {
      Serial.print((char)Serial1.read());
      if(Serial1.find(send)) {
        flag = true;
        break;
      }
      if (flag)
        break;
    }
  }  

  if (DEBUG)    
    Serial.print(flag);

  if (flag)
    Serial1.print(buffer_ptr);

  return;
}

void loop() {
  /* store the channel_id and packet_len */
  int channel_id, packet_len;
  /* point to valid data in buffer */
  char *buffer_ptr;

  /* read the data */
  if (readTillEol()) {
    /* make sure the buffer has +IPD, ie data from client */
    if(strncmp(buffer, "+IPD,", 5)==0) {
      /* get the channel_id and packet_len
         the buffer format is IPD+,channel_id,&packet_len:DATA*/
      sscanf(buffer+5, "%d,%d", &channel_id, &packet_len);
      if (packet_len > 0) {
        /* jump +IPD, */
        buffer_ptr = buffer+5;
        /* start reading from ':' (skip the channel_id and packet_len) */
        while(*buffer_ptr++!=':');
        /* echo back */
        echo_back(channel_id, packet_len, buffer_ptr, 10000);        
    } else {
        /* TODO: oops, we got some other data. do errors processing */
        if (DEBUG)
          Serial.println("ERR: IPD+");
      }
    }
  }
}
