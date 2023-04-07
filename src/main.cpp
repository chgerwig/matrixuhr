/* Matrixuhr von chgerwig@gmail.com Version 0.9.5 vom 2.4.2023

 Hardware: ESP12E
 Funktionen:
 - Verbindet sich mit dem WLAN
 - Holt sich die ntp Zeit vom Internet
 - Zeigt die Zeit im 24 Stundenformat an
 - Ändern der Schriftart
 - Automatische Sommerzeit Umstellung
*/ 

#include <MD_MAX72xx.h>
#include <SPI.h>
//#include <NTPClient.h>
#include <time.h>
// change next line to use with another board/shield
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char *ssid     = "openiot";
const char *password = "lorawahn";

/* Configuration of NTP */
#define MY_NTP_SERVER "at.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   

time_t now;                         // this is the epoch
tm tm;                              // the structure tm holds time information in a more convient way

//WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP);

#define PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4 // Anzahl 8x8 Module

#define CLK_PIN   14  // or SCK
#define DATA_PIN  13 // or MOSI
#define CS_PIN    12  // or SS

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary pins
//MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Text parameters
#define CHAR_SPACING  1 // pixels between characters

// Global message buffers shared by Serial and Scrolling functions
#define BUF_SIZE  10
char message[BUF_SIZE] = "Suche...";
bool newMessageAvailable = true;

void printText(uint8_t modStart, uint8_t modEnd, char *pMsg)
// Print the text string to the LED matrix modules specified.
// Message area is padded with blank columns after printing.
{
  uint8_t   state = 0;
  uint8_t   curLen;
  uint16_t  showLen;
  uint8_t   cBuf[8];
  int16_t   col = ((modEnd + 1) * COL_SIZE) - 1;

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  do     // finite state machine to print the characters in the space available
  {
    switch(state)
    {
      case 0: // Load the next character from the font table
        // if we reached end of message, reset the message pointer
        if (*pMsg == '\0')
        {
          showLen = col - (modEnd * COL_SIZE);  // padding characters
          state = 2;
          break;
        }

        // retrieve the next character form the font file
        showLen = mx.getChar(*pMsg++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);
        curLen = 0;
        state++;
        // !! deliberately fall through to next state to start displaying

      case 1: // display the next part of the character
        mx.setColumn(col--, cBuf[curLen++]);

        // done with font character, now display the space between chars
        if (curLen == showLen)
        {
          showLen = CHAR_SPACING;
          state = 2;
        }
        break;

      case 2: // initialize state for displaying empty columns
        curLen = 0;
        state++;
        // fall through

      case 3:	// display inter-character spacing or end of message padding (blank columns)
        mx.setColumn(col--, 0);
        curLen++;
        if (curLen == showLen)
          state = 0;
        break;

      default:
        col = -1;   // this definitely ends the do loop
    }
  } while (col >= (modStart * COL_SIZE));

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void resetMatrix(void)
{
  mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY/8); //Anzeigehelligkeit
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  mx.clear();
}

void setup()
{
  mx.begin();
  resetMatrix();
  printText(0, MAX_DEVICES-1, message);

  Serial.begin(115200);
  delay(5000); // wait for serial connection
  Serial.println("connecting");
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }
  for(int i=6; i<0; i--){
    message[i] = char(32);
  }

  configTime(MY_TZ, MY_NTP_SERVER);

  printText(0, MAX_DEVICES-1, message);
  //timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  //timeClient.setTimeOffset(3600);

  Serial.print("/n[MD_MAX72XX Message Display]");
}
void loop()
{
  //timeClient.update();
  time(&now);                       // read the current time
  localtime_r(&now, &tm);
  String Stunde;
  String Minute;
// Wenn Stunde einstellig, dann eine 0 voranstellen
  if (tm.tm_hour < 10){
    Stunde = "0" + String(tm.tm_hour);
  }else{
    Stunde = String(tm.tm_hour);
  }
// Wenn Minute einstellig, dann eine 0 voranstellen
  if (tm.tm_min < 10){
    Minute = "0" + String(tm.tm_min);
  }else{
    Minute = String(tm.tm_min);
  }
  String formattedTime = Stunde + ":" + Minute;
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);  
  //prepare time for matrix
  String Zeit = formattedTime.substring(0,5);
  Serial.print("timeString:");
  Serial.println(Zeit);
  formattedTime.toCharArray(message, 6);
  Serial.print("Message:");
  Serial.println(message);
  printText(0, MAX_DEVICES-1, message);
  delay(1000);
}