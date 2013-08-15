#include <Servo.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

//Internet stuff
byte mac[] = { 0x8E, 0x8D, 0xBE, 0x8F, 0xFE, 0xEE };
//char server[] = "ec2-54-213-123-121.us-west-2.compute.amazonaws.com";
char server[] = "10.32.25.80";

//Clients
EthernetClient client;
PubSubClient mqclient(server, 1883, callback, client);

//Servos
Servo myservo8;
Servo myservo9;
int inValue = -1;
int outValue = -1;

//Strings
char contype[] = "Content-Type: application/x-www-form-urlencoded";
char conclose[] = "Connection: close";
char conlen[] = "Content-Length: 20";
char topic[] = "sens/ ";

//Various
int sensorNum = -1;
int debugInfoTCP = 0;
int debugInfoUDP = 0;
int moreDebugInfo = 0;
int delayAmount = 20;

void setup(){
  Serial.begin(9600);
  Serial.print('.');
  if(Ethernet.begin(mac)==0){
    Serial.println("Failed to configure Ethernet with DHCP");
    while(true);
  }
  else Serial.println("Connected to Ethernet");
  delay(1000);
  
  getMyNum();
  
  topic[5] = (char)(sensorNum+48);
  
  while(!mqclient.connect("Ardyout")){
    Serial.println("Connection failed, trying again in 1s");
    delay(1000);
  }
  
  mqclient.subscribe(topic);
  
  myservo8.attach(8);
  myservo9.attach(9);
}


void loop(){
  //delay(delayAmount);
  if(!mqclient.loop()){
    mqconnect();
    mqclient.subscribe(topic);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  /*Serial.print("bytes:");
  Serial.println((char*)payload);
  Serial.print("Length:");
  Serial.println(length);*/
  int inValue = arrayChop(payload,length);
  //Serial.println(inValue);
  //if(inValue !=-1){
    //Serial.println("Yay");
    if(inValue>=0 && inValue<=1024){
      outValue = map(inValue,0,1023,0,179);
      Serial.println(outValue);
      myservo8.write(outValue);
      myservo9.write(outValue);
  //  }
  }

}

int arrayChop(byte* payload, unsigned int length){
/*  char ret[length+1];
  for(int i = 0;i<length;i++){
    ret[i] = (char)(*(payload+i));
  }
  ret[length] = '\0';
  //Serial.println(ret);*/
  return myParse((char*)payload,length);
}

void mqconnect(){
  Serial.println("Connecting");
  while(!mqclient.connect("Ardyout")){
    Serial.println("Connection failed, trying again in 1s");
    delay(1000);
  }
}

int myParse(char buf[], int len){
  int i = 0;
  int ret = -1;
  for(i;i<len;i++){
    if(buf[i] >= '0' && buf[i] <= '9'){
      if(ret==-1)
        ret=buf[i]-48;
      else{
        ret*=10;
        ret+=buf[i]-48;
      }
    }
    else break;
    
  }
  return ret;
}

//Initial thing to be called; gets the sensor number to watch via TCP, right now just the last one
void getMyNum(){
  TCPConnect();
  
  //Send http request
  client.println("GET /sensors/num HTTP/1.0");
  client.println(contype);
  client.println(conlen);
  client.println(conclose);
  client.println();
  client.print("ip=");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    client.print(Ethernet.localIP()[thisByte], DEC);
    client.print("."); 
  }
  client.println();
  client.println();
  Serial.println("Supposedly sent request");
  
  //Parse reply
  char c = 0;
  int lastNum = -1;
  while(sensorNum ==-1){  
    while(client.available()){
      c = client.read();
      if(c>='0' && c<='9'){
        lastNum = c-48;
        if(debugInfoTCP)
          Serial.print("#");
      }
      if(debugInfoTCP)
        Serial.print(c);
    }
    if(sensorNum==0)
      while(true);
    if(lastNum != -1)
      sensorNum = lastNum-1;
    else delay(50);
  }
  if(debugInfoTCP)
    Serial.println();
  Serial.print("Got sensor number: ");
  Serial.println((char)(sensorNum+48));
  TCPStop();
}

//Ends TCP Connection
void TCPStop(){
  while(client.available())
    client.read();
  client.flush();
  if(!client.connected()){
    client.stop();
    Serial.println("Stopping TCP...");
  }
}

//Connects to server via TCP
void TCPConnect(){
  Serial.println("connecting TCP...");
  while(!client.connect(server,4000)){
    Serial.println("Connection failed, trying again in 1s");
    delay(1000);
  }
  if(debugInfoTCP)
    Serial.println("connected TCP");
}





