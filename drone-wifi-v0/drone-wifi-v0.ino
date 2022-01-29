//#include "painlessMesh.h"
#include "namedMesh.h"
#include <ArduinoJson.h>
#include "WiFiGeneric.h" // to redude the power

#define   MESH_PREFIX     "gradysnetwork"
#define   MESH_PASSWORD   "yetanothersecret"
#define   MESH_PORT       8888
#define   myChipNameSIZE    24
#define   LED             5       // GPIO number of connected LED, ON ESP-12 IS GPIO2

Scheduler userScheduler; // to control tasks
//painlessMesh  mesh;
namedMesh  mesh;

char myChipName[myChipNameSIZE]; // Chip name (to store MAC Address
String myChipStrName = "drone-B";
//String nodeName(myChipStrName); // Name needs to be unique and uses the attached .h file

SimpleList<uint32_t> nodes; // painlessmesh
boolean iAmConnected = false;

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


void sendDataToDrone() {
  sendMessage();
}

void sendDataResquestToSensor() {
  sendMessage();
  //taskSendDataRequest.setInterval(TASK_SECOND * 1);  // between 1 and 5 seconds
}

void sendStatsToGSr() {
  String msg = " I got ";
  //msg += myChipStrName;
  msg += "-> " + String(msgsReceived);
  msg += " msgs so far.";
  if (iAmConnected) {
    mesh.sendBroadcast( msg );
    msgsSent++;
  }
}

void sendMessage() {
  if (iAmConnected) {
    mesh.sendBroadcast(myChipStrName);
    msgsSent++;
  }
}

//void receivedCallback( uint32_t from, String &msg ) {
//  Serial.printf("%s: Received from %u msg=%s\n", myChipName, from, msg.c_str());
//}

void receivedCallback_str(String &from, String &msg ) {

  if(not(from.indexOf(myChipStrName) >= 0)) { // naive loopback avoiddance

    if(from.indexOf("drone") >= 0) {
        doNothing();
    } 
    else if(from.indexOf("sensor") >= 0) {
        if(myChipStrName.indexOf("drone") >= 0){ 
          msgsReceived++;
          sendDataResquestToSensor();
          taskSendDataRequest.disable();
        } else {
          doNothing();
        }
    } 
    else if(from.indexOf("GS") >= 0) {
        if(myChipStrName.indexOf("drone") >= 0){ 
          sendStatsToGSr();
          taskSendDataRequest.disable();
          Serial.printf("%s: Msg from %s : %s\n", myChipStrName, from.c_str(), msg.c_str());        
        }         
    } else {
        Serial.printf("%s: UNKWON PLAYER from %s msg=%s\n", myChipStrName, from.c_str(), msg.c_str());
    }
  }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("%s: New Connection, nodeId = %u\n", myChipStrName, nodeId);
}

void changedConnectionCallback() {
  Serial.printf("%s: Changed connections\n", myChipStrName);

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  if(nodes.size()>0){
    //sendDataResquestToSensor();
    iAmConnected = true;
    digitalWrite(LED, HIGH);
    Serial.printf("%s: Turnning the led ON (connection active)\n", myChipStrName);
  }else{
    iAmConnected = false;
    taskSendDataRequest.enable();
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

  userScheduler.addTask( taskSendDataRequest );
  if(myChipStrName.indexOf("drone") >= 0){ 
    taskSendDataRequest.enable();
  }

  WiFi.setTxPower(WIFI_POWER_5dBm );

  pinMode (LED, OUTPUT);
  digitalWrite(LED, LOW);
  Serial.printf("%s: Turnning the led OFF\n", myChipStrName);
 
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
  //int i = WiFi.getTxPower();    
  //Serial.printf("dBm = %d\n",i);

  if ((msgsReceived % 1001 == 0)&&(msgsReceived > 0)){
     Serial.printf("%s: Acc received %d\n", myChipStrName, msgsReceived);       
  }  
//  if ((msgsSent % 1002 == 0)&&(msgsSent > 0)){
//     Serial.printf("%s: Acc msg sent %d\n", myChipStrName, msgsSent);  
//  }
//  
  
}


//typedef enum {
//    WIFI_POWER_19_5dBm = 78,// 19.5dBm
//    WIFI_POWER_19dBm = 76,// 19dBm
//    WIFI_POWER_18_5dBm = 74,// 18.5dBm
//    WIFI_POWER_17dBm = 68,// 17dBm
//    WIFI_POWER_15dBm = 60,// 15dBm
//    WIFI_POWER_13dBm = 52,// 13dBm
//    WIFI_POWER_11dBm = 44,// 11dBm
//    WIFI_POWER_8_5dBm = 34,// 8.5dBm
//    WIFI_POWER_7dBm = 28,// 7dBm
//    WIFI_POWER_5dBm = 20,// 5dBm
//    WIFI_POWER_2dBm = 8,// 2dBm
//    WIFI_POWER_MINUS_1dBm = -4// -1dBm
//} wifi_power_t;
