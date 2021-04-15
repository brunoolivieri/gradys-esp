//#include "painlessMesh.h"
#include "namedMesh.h"
#include <ArduinoJson.h>

#define   MESH_PREFIX     "gradysnetwork"
#define   MESH_PASSWORD   "yetanothersecret"
#define   MESH_PORT       8888
#define   myChipNameSIZE    24
#define   LED             5       // GPIO number of connected LED, ON ESP-12 IS GPIO2

Scheduler userScheduler; // to control tasks
//painlessMesh  mesh;
namedMesh  mesh;

char myChipName[myChipNameSIZE]; // Chip name (to store MAC Address
String myChipStrName = "GS-A";
//String nodeName(myChipStrName); // Name needs to be unique and uses the attached .h file

SimpleList<uint32_t> nodes; // painlessmesh

// stats
uint32_t msgsReceived = 0;
uint32_t msgsSent = 0;



// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
void sendDataResquestToSensor() ; // Prototype to schedule doesn't complain

Task taskSendDataRequest( TASK_SECOND * 1 , TASK_FOREVER, &sendDataResquestToSensor);

void doNothing() {
  // TO-DO: a health checking
  int i = random(17171);
}

boolean isPrime(uint64_t x) {
  boolean prime = true;
  for(uint64_t i = 2; i <= x/2; i++) { //Loop every number up to half
    if(x % i == 0) { //If it's divisible...
      prime = false; //It isn't prime!
      break;
    }
  }
  return prime;
}

void naivelyEnablePreemption() {

  //uint64_t randNumber = random(18446744073709551615);
  for (uint64_t i = 0; i < 18446744073709551615; i++){
    //if (isPrime(uint64_t x)){
      doNothing();
    //}
  }
}

void sendDataToDrone() {
  sendMessage();
}

void sendDataResquestToSensor() {
  sendMessage();
  //taskSendDataRequest.setInterval(TASK_SECOND * 1);  // between 1 and 5 seconds
}

void sendDataResquestToDrone() {
  sendMessage();
}


void sendStatsToGSr() {
  
  String msg = " I got from ";
  //msg += myChipStrName;
  msg += "-> " + String(msgsReceived);
  mesh.sendBroadcast( msg );
  msgsSent++;
  naivelyEnablePreemption();// terrible workaround to provoce preemption between async_tcp and others. otherwise watchdog kills the connection to prevent starvation
  // tries to prevent sensor to reboot with large transfers
}

void sendMessage() {
  mesh.sendBroadcast(myChipStrName);
  msgsSent++;
  naivelyEnablePreemption(); // terrible workaround to provoce preemption between async_tcp and others. otherwise watchdog kills the connection to prevent starvation
  // tries to prevent sensor to reboot with large transfers
}


//void receivedCallback( uint32_t from, String &msg ) {
//  Serial.printf("%s: Received from %u msg=%s\n", myChipName, from, msg.c_str());
//}

void receivedCallback_str(String &from, String &msg ) {

  msgsReceived++;
  if(not(from.indexOf(myChipStrName) >= 0)) { // naive loopback avoiddance

    if(from.indexOf("drone") >= 0) {
       sendDataResquestToDrone();
       doNothing();
       Serial.printf("%s: Msg %s : %s\n", myChipStrName, from.c_str(), msg.c_str());
    } 
    else if(from.indexOf("sensor") >= 0) {
        doNothing();
    } 
    else if(from.indexOf("GS") >= 0) {
        doNothing();
    } else {
        Serial.printf("%s: UNKWON PLAYER from %s :%s\n", myChipStrName, from.c_str(), msg.c_str());
    }
  }
  naivelyEnablePreemption(); // terrible workaround to provoce preemption between async_tcp and others. otherwise watchdog kills the connection to prevent starvation  
  // tries to prevent sensor to reboot with large transfers
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("%s: New Connection, nodeId = %u\n", myChipStrName, nodeId);
}

void changedConnectionCallback() {
  Serial.printf("%s: Changed connections\n", myChipStrName);

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  if(nodes.size()>0){
    digitalWrite(LED, HIGH);
    Serial.printf("%s: Turnning the led ON (connection active)\n", myChipStrName);
  }else{
    digitalWrite(LED, LOW);
    Serial.printf("%s: Turnning the led OFF (no connections)\n", myChipStrName);
  }
  
  Serial.printf("Connection list:");
  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("%s: Adjusted time %u. Offset = %d\n", myChipStrName, mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);

  Serial.print("Hello! I am ");
  Serial.println(myChipStrName);

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );

  mesh.setName(myChipStrName); // This needs to be an unique name! 
  //mesh.onReceive(&receivedCallback);
  mesh.onReceive(&receivedCallback_str); //extra for names
  
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  WiFi.setTxPower(WIFI_POWER_7dBm );

  pinMode (LED, OUTPUT);
  digitalWrite(LED, LOW);
  Serial.printf("%s: Turnning the led OFF\n", myChipStrName);
  
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();

  //int i = WiFi.getTxPower();    
  //Serial.printf("dBm = %d\n",i);

  if ((msgsReceived % 1000 == 0)&&(msgsReceived > 0)){
     Serial.printf("%s: Acc received %d\n", myChipStrName, msgsReceived);       
  }  
  if ((msgsSent % 1000 == 0)&&(msgsSent > 0)){
     Serial.printf("%s: Acc msg sent %d\n", myChipStrName, msgsSent);  
  }

  

}
