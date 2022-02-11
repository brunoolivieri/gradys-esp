// Version otimized for https://github.com/LilyGO/TTGO-T2-SSD1331-SD   v.16

#define FS_NO_GLOBALS   // Because: It is unable to use both SPIFFS and SD : https://github.com/homieiot/homie-esp8266/pull/462
#include <mySD.h>       // http://www.iotsharing.com/2017/05/how-to-use-arduino-esp32-to-store-data-to-sdcard.html
#include "namedMesh.h"  // Is uses SPIFFS
#include <ArduinoJson.h>
#include "WiFiGeneric.h" // to redude the power

#define   MESH_PREFIX     "gradysnetwork"
#define   MESH_PASSWORD   "yetanothersecret"
#define   MESH_PORT       8888
#define   myChipNameSIZE  24
#define   LED             5 // GPIO number of connected LED, ON ESP-12 IS GPIO2, On ESP32+batt (t18 is 5)
// This board (TTGO-t2 v1.6) uses LED low to turn it on and high to turn it off


Scheduler userScheduler; // to control tasks
namedMesh  mesh;

// test command to use at the Lab
String commandFromDrone;

DynamicJsonDocument json_received(1024);//Deserialized str to json from GS broadcast
StaticJsonDocument<256> json_to_send; //JSON to send ACK
char json_string[256];

char myChipName[myChipNameSIZE]; // Chip name (to store MAC Address
String myChipStrName = "uav-A";

SimpleList<uint32_t> nodes; // painlessmesh
boolean iAmConnected = false;

long randNumber;

// stats
uint32_t dataMsgReceived = 0;
uint32_t gsMsgReceived = 0;
uint32_t msgsReceived = 0;
uint32_t msgsSent = 0;
uint8_t maxConnections = 0;

// File mgmt
//File logFile;
//File debugFile;
char *testID = "T0";
char *dateStr = "1900-01-22";
char logFileName[] = "default.txt";
char debugFileName[] = "debug.txt";


// User stub
void sendMessage() ; // Prototype, so PlatformIO doesn't complain
void sendDataResquestToSensor() ; // Prototype, so PlatformIO doesn't complain

Task taskSendDataRequest( TASK_SECOND * 1 , TASK_FOREVER, &sendDataResquestToSensor);

void doNothing() {
  // TO-DO: a health checking
  int i = random(17171);
}

void openSDcard(){
  Serial.println("Initializing SD card...");
  /* initialize SD library with Soft SPI pins, if using Hard SPI replace with this SD.begin()*/
  if (!SD.begin(13, 14, 2, 14)) {  // 13=CS; 15=MOSI; 2=MISO; 14=SCK
    Serial.println("SD Card - init failed!");
    return;
  }
  Serial.println("SD Card - init OK.");    
}

int countFilesInSDcard(){
  int accFiles = 0;
  logFile = SD.open("/");
  if (logFile) {    
    while(true) {
    File entry =  logFile.openNextFile();
     if (!entry) {
       break;
     }
     accFiles++;
     entry.close();
    }
    logFile.close();
  } else {
    Serial.println("error couting files");
  }  
  return accFiles; 
}

void doLog2SDCard(String logline){
  Serial.printf("I should log: %s in %s\n", logline, logFileName);
  auto logFile = SD.open(logFileName, FILE_WRITE);
  /* if open succesfully -> myFile != NULL  */
  if (logFile) {
    logFile.println(logline);
    logFile.flush();
    logFile.close();
  } else {
    Serial.println("ERROR logging");
  }   
}

//void debugNote(String logline){
//  Serial.printf("I should log: %s in %s\n", logline, debugFileName);
//  debugFile = SD.open(debugFileName, FILE_WRITE);
//  if (debugFile) {
//    debugFile.println(logline);
//    debugFile.flush();
//    debugFile.close();
//  } else {
//    Serial.println("ERROR logging");
//  }   
//}

void sendDataToDrone() {
  sendMessage();
}

void sendDataResquestToSensor() {
  sendMessage();
  //taskSendDataRequest.setInterval(TASK_SECOND * 1);  // between 1 and 5 seconds
}

void sendMessage() {
  if (iAmConnected) {
    mesh.sendBroadcast(myChipStrName);
    msgsSent++;
  }
}

void receivedCallback_str(String &from, String &msg ) {

  msgsReceived++;
  if(not(from.indexOf(myChipStrName) >= 0)) { // naive loopback avoidance

    if(from.indexOf("uav") >= 0) {
        doNothing();
    } 
    else if(from.indexOf("sensor") >= 0) {
        if(myChipStrName.indexOf("uav") >= 0){ 
          dataMsgReceived++;
          sendDataResquestToSensor();
          //taskSendDataRequest.disable();
        } else {
          doNothing();
        }
    } 
    else if(from.indexOf("gs") >= 0) {      
      if(myChipStrName.indexOf("uav") >= 0){ 
          //sendStatsToGSr();
          //taskSendDataRequest.disable();
          gsMsgReceived++;
          Serial.printf("%s: Msg from %s : %s\n", myChipStrName, from.c_str(), msg.c_str());
          JsonObject root = json_to_send.to<JsonObject>(); 
          digitalWrite(LED,LOW);
          root["id"] = myChipStrName;
          root["msgsReceived"] = msgsReceived;
          root["msgsSent"] = msgsSent;
          root["dataMsgReceived"] = dataMsgReceived;
          root["gsMsgReceived"] = gsMsgReceived;
          root["maxConnections"] = maxConnections;          
          serializeJson(root, json_string);
          if (iAmConnected) {
            mesh.sendBroadcast(json_string);
          }
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
  if(nodes.size()>maxConnections){
    maxConnections = nodes.size();
  }
  Serial.printf("Num nodes: %d\n", nodes.size());
  if(nodes.size()>0){
    //sendDataResquestToSensor();
    iAmConnected = true;
    digitalWrite(LED, LOW);
    Serial.printf("%s: Turnning the led ON (connection active)\n", myChipStrName);
  }else{
    iAmConnected = false;
    taskSendDataRequest.enable();
    digitalWrite(LED, HIGH);
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

  // NETWORK setup //////////////////////////////////////////////////////
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.setName(myChipStrName); // This needs to be an unique name! 
  //mesh.onReceive(&receivedCallback);
  mesh.onReceive(&receivedCallback_str); //extra for names 
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  userScheduler.addTask( taskSendDataRequest);
  if(myChipStrName.indexOf("uav") >= 0){ 
    taskSendDataRequest.enable();
  }
  WiFi.setTxPower(WIFI_POWER_11dBm); // Specific for the experiment
  pinMode (LED, OUTPUT);
  digitalWrite(LED, HIGH);
  Serial.printf("%s: Turnning the led OFF\n", myChipStrName);

  // SD card setup //////////////////////////////////////////////////////
  openSDcard();
  int acc_files = countFilesInSDcard();
  Serial.printf("\nFiles found: %s\n", String(acc_files));
  acc_files++;
  strcpy(logFileName,testID);
  strcat(logFileName,"-");
  char temp[12]; 
  itoa(acc_files, temp, 10);
  strcat(logFileName,temp);
  strcat(logFileName,".txt");  
  Serial.printf("\n\nNew logfile name: \\%s\\\n\n",logFileName );
  //debugNote("bootstrap");
  randomSeed(analogRead(0));
  Serial.println("Finish init");

  
}

void loop() {
  while(Serial.available()){
    commandFromDrone = Serial.readString( );
    if (iAmConnected) {
      mesh.sendBroadcast(commandFromDrone);
    }
  }
  
  // it will run the user scheduler as well
  mesh.update();

  if ((dataMsgReceived % 1001 == 0)&&(dataMsgReceived > 0)){
     Serial.printf("%s: dataMsgReceived received %d\n", myChipStrName, dataMsgReceived);       
  }  
  if ((msgsReceived % 1002 == 0)&&(msgsReceived > 0)){
     Serial.printf("%s: msgsReceived received %d\n", myChipStrName, msgsReceived);       
  }  
  if ((msgsSent % 1003 == 0)&&(msgsSent > 0)){
     Serial.printf("%s: msgsSent msg sent %d\n", myChipStrName, msgsSent);  
     randNumber = random(10, 20);
     String thisString = String(randNumber);
     doLog2SDCard(thisString);
     //debugNote(thisString);

  }


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
