#define DEBUG 1
#define PORT 8888

#define SSID     "xxxxx"
#define SSPASSWD "XXXXXXX"

char rn[] = "\r\n";
char ok[] = "OK";
char ready[] = "ready";

/* sends a command and test whether we got the ack string */
boolean checkSendDataAck(String command, char *ack, const int timeout) {
  boolean flag = false;              /* did we see the ack string */

  if (DEBUG)
    Serial.print(command);
  
  /* set timeout */
  Serial1.setTimeout(timeout);
  
  /* write to module */
  Serial1.print(command); // send the read character to the Serial1

  if(Serial1.find(ack)) {          
        flag = true;
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
    while(Serial1.available()) {
      char data = (char)Serial1.read();
      Serial.write(data);
    }
  }

  return;
}

/* sends a command and test whether we got the ack string */
boolean clearBuffer(const int timeout) {
  boolean flag = false;
  /* set timeout */
  Serial1.setTimeout(timeout);

  if(Serial1.find(rn)) {
    flag = true;
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
  status = clearBuffer(1000);

  /* if not connected, try to connect */
  if (not status) {
    Serial.println("Not connected to WiFi.. Trying to Connect");
      command = "AT+CWJAP==\"";
      command += SSID;
      command += "\",\"";
      command += SSPASSWD;
      command += "\"\r\n";
      status = false; retry = 4;
      while((status = checkSendDataAck(command, ok, 2000)) == false && retry-- > 0)
        if (DEBUG)
          Serial.println("retry");
  }

  /* get ip from */
  String ip;
  status = getIpAddress(1000, &ip);


  /* allow it for multiconnection */
  command = "AT+CIPMUX=1\r\n";
  while((status = checkSendDataAck(command, ok, 2000)) == false && retry-- > 0)
    if (DEBUG)
      Serial.println("retry");

  /* start the server at given port (1 = create, PORT = port) */
  command = "AT+CIPSERVER=1,";
  command += PORT;
  command += "\r\n";
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
  if (DEBUG) {
    Serial.println(status);
  } else {
    while(1);
  }

  Serial.println("Started Server");
}

void loop() {
  if (Serial1.available()) {
    Serial.write(Serial1.read());
  }
}