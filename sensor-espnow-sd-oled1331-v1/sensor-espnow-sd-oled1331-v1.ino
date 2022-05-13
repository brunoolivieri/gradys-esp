// used with ttgo t2 v.16 w/ oled 1331 and SD Card

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#include <mySD.h>


// The SSD1331 is connected like this (plus VCC plus GND)
const uint8_t   OLED_pin_scl_sck        = 14;
const uint8_t   OLED_pin_sda_mosi       = 13;
const uint8_t   OLED_pin_cs_ss          = 15;
const uint8_t   OLED_pin_res_rst        = 4;
const uint8_t   OLED_pin_dc_rs          = 16;

// SSD1331 color definitions
const uint16_t  OLED_Color_Black        = 0x0000;
const uint16_t  OLED_Color_Blue         = 0x001F;
const uint16_t  OLED_Color_Red          = 0xF800;
const uint16_t  OLED_Color_Green        = 0x07E0;
const uint16_t  OLED_Color_Cyan         = 0x07FF;
const uint16_t  OLED_Color_Magenta      = 0xF81F;
const uint16_t  OLED_Color_Yellow       = 0xFFE0;
const uint16_t  OLED_Color_White        = 0xFFFF;

// The colors we actually want to use
uint16_t        OLED_Text_Color         = OLED_Color_Black;
uint16_t        OLED_Backround_Color    = OLED_Color_White;

// declare the display
Adafruit_SSD1331 oled =
    Adafruit_SSD1331(
        OLED_pin_cs_ss,
        OLED_pin_dc_rs,
        OLED_pin_sda_mosi,
        OLED_pin_scl_sck,
        OLED_pin_res_rst
     );

// assume the display is off until configured in setup()
bool            is_display_visible        = false;

// declare size of working string buffers. Basic strlen("d hh:mm:ss") = 10
const size_t    max_string_size               = 16;

// the string being displayed on the SSD1331 (initially empty)
char old_time_string[max_string_size]           = { 0 };
char old_rx_string[max_string_size]           = { 0 };
char old_tx_string[max_string_size]           = { 0 };

// File mngt
File my_file;
char *testID = "T0";
char log_filename[] = "default.txt";
long rand_number;

// Experiment variables
unsigned long  count_rx_msgs = 0;
unsigned long  count_tx_msgs = 0;



///////////////////////////////////////////////////////////////////////
//
//  Screen section
//
///////////////////////////////////////////////////////////////////////
// updates every 1 second and call other updates
void display_refresh() {

    unsigned long uptime_seconds = millis() / 1000;
    unsigned long days = uptime_seconds / 86400;
    uptime_seconds = uptime_seconds % 86400;
    unsigned long hours = uptime_seconds / 3600;
    uptime_seconds = uptime_seconds % 3600;
    unsigned long minutes = uptime_seconds / 60;
    uptime_seconds = uptime_seconds % 60;
    char new_time_string[max_string_size] = { 0 };

    // construct the string representation
    sprintf(
        new_time_string,
        //"%lu %02lu:%02lu:%02lu",
        //days, hours, minutes, uptime_seconds
        "%01lu:%02lu:%02lu",
        hours, minutes, uptime_seconds
    );

    // has the time string changed since the last oled update?
    if (strcmp(new_time_string,old_time_string) != 0) {

        // yes! home the cursor
        oled.setCursor(5,5);
        // change the text color to the background color
        oled.setTextColor(OLED_Backround_Color);
        // redraw the old value to erase
        oled.print(old_time_string);
        // home the cursor
        oled.setCursor(5,5);       
        // change the text color to foreground color
        oled.setTextColor(OLED_Text_Color);
        // draw the new time value
        oled.print(new_time_string);
        // and remember the new value
        strcpy(old_time_string,new_time_string);

        display_tx_rx_data(); //rx and tx data

    }

}

// display the device ID
void display_dev_name(){
   oled.setCursor(5,20);  
   oled.setTextColor(OLED_Text_Color);
   oled.print("Sensor 7");
}

// display RX/TX number of messages
void display_tx_rx_data(){
    char new_rx_string[max_string_size] = { 0 };
    char new_tx_string[max_string_size] = { 0 };
    // construct the stringSS representation
    sprintf(
        new_rx_string,
        "RX %01lu",
        count_rx_msgs
    );
    sprintf(
        new_tx_string,
        "TX %01lu",
        count_tx_msgs
    );
    if (strcmp(new_rx_string,old_rx_string) != 0) {
       // yes! home the cursor
        oled.setCursor(5,35);
        oled.setTextColor(OLED_Backround_Color);
        oled.print(old_rx_string);
        oled.setCursor(5,35);       
        oled.setTextColor(OLED_Text_Color);
        oled.print(new_rx_string);
        strcpy(old_rx_string,new_rx_string);
    }
    if (strcmp(new_tx_string,old_tx_string) != 0) {
       // yes! home the cursor
        oled.setCursor(5,50);
        oled.setTextColor(OLED_Backround_Color);
        oled.print(old_tx_string);
        oled.setCursor(5,50);       
        oled.setTextColor(OLED_Text_Color);
        oled.print(new_tx_string);
        strcpy(old_tx_string,new_tx_string);
    }
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////
//
//  SD Card section
//
///////////////////////////////////////////////////////////////////////
void open_sdcard(){
  Serial.println("Initializing SD card...");
  /* initialize SD library with Soft SPI pins, if using Hard SPI replace with this SD.begin()*/
  if (!SD.begin(5, 23, 19, 18)) {  // 5=CS; 23=MOSI; 19=MISO; 18=SCK
    Serial.println("SD Card - init failed!");
    return;
  }
  Serial.println("SD Card - init OK.");    
}
int count_sdcard_files(){
  int counter_files = 0;
  my_file = SD.open("/");
  if (my_file) {    
    while(true) {
     File entry =  my_file.openNextFile();
     if (!entry) {
       break;
     }
     counter_files++;
     entry.close();
    }
    my_file.close();
  } else {
    Serial.println("error couting files");
  }  
  return counter_files; 
}
void log_2_sdcard(String logline){
  Serial.printf("I should log: %s in %s\n", logline, log_filename);

  /* open "test.txt" for writing */
  my_file = SD.open(log_filename, FILE_WRITE);
  /* if open succesfully -> my_file != NULL 
    then write string "Hello world!" to it
  */
  if (my_file) {
    my_file.println(logline);
    my_file.flush();
    my_file.close();
  } else {
    Serial.println("ERROR logging");
  }   
}
///////////////////////////////////////////////////////////////////////



void setup() {

    Serial.begin(115200); while (!Serial); Serial.println();

    // settling time
    delay(250);

    // initialise the SSD1331
    oled.begin();
    oled.setFont();
    oled.fillScreen(OLED_Backround_Color);
    oled.setTextColor(OLED_Text_Color);
    oled.setTextSize(1.8);
    is_display_visible = true;
    display_dev_name();
    ///////////////////////////////////////////////////////////////////////
    // SD Card Stuff
    open_sdcard();
    int count_files = count_sdcard_files() - 1; // novidade não testada
    Serial.printf("\nFiles found: %s\n", String(count_files));
    count_files++;
  
    strcpy(log_filename,testID);
    strcat(log_filename,"-");
    char temp[12]; 
    itoa(count_files, temp, 10);
    strcat(log_filename,temp);
    strcat(log_filename,".txt");
    
    Serial.printf("\n\nNew logfile name: \\%s\\\n\n",log_filename );
    log_2_sdcard("bootstrap");
    randomSeed(analogRead(0));
    Serial.println("Finish init");
    ///////////////////////////////////////////////////////////////////////
    
}


void loop() {

    display_refresh();
    delay(100);
    
    // only for testing
    count_rx_msgs++; 
    count_tx_msgs = rand_number;

    // sd card tests
    rand_number = random(10, 20);
    String some_string = String(rand_number);
    log_2_sdcard(some_string);
    delay(2000);
    
}
