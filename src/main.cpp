// Use the MD_MAX72XX library to scroll text on the display
// received through the ESP8266 WiFi interface.
//
// Demonstrates the use of the callback function to control what
// is scrolled on the display text. User can enter text through
// a web browser and this will display as a scrolling message on
// the display.
//
// IP address for the ESP8266 is displayed on the scrolling display
// after startup initialization and connected to the WiFi network.
//
// Connections for ESP8266 hardware SPI are:
// Vcc       3v3     LED matrices seem to work at 3.3V
// GND       GND     GND
// DIN        D7     HSPID or HMOSI
// CS or LD   D8     HSPICS or HCS
// CLK        D5     CLK or HCLK
//

#define COMPDATE __DATE__ __TIME__
#define MODEBUTTON 0                                  // Button pin on the esp for selecting modes. D3 for the Wemos!



#include <Arduino.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <IOTAppStory.h>
#include <WiFi.h>

#include <AsyncTCP.h>                               // https://github.com/me-no-dev/AsyncTCP

#include <SPIFFS.h>

#include <ESPAsyncWebServer.h>                        // https://github.com/me-no-dev/ESPAsyncWebServer
#include <FS.h>

#define PRINT_CALLBACK  0
#define DEBUG 1
#define LED_HEARTBEAT 0

#if DEBUG
#define PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }
#define PRINTS(s)   { Serial.print(F(s)); }
#else
#define PRINT(s, v)
#define PRINTS(s)
#endif


#if LED_HEARTBEAT
#define HB_LED  D2
#define HB_LED_TIME 500 // in milliseconds
#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 12

#define CLK_PIN   SCK // or SCK
#define DATA_PIN  MOSI // or MOSI
#define CS_PIN    SS // or SS

IOTAppStory IAS(COMPDATE,MODEBUTTON);


// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary pins
//MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// WiFi Server object and parameters
AsyncWebServer server(80);

// Global message buffers shared by Wifi and Scrolling functions
const uint8_t MESG_SIZE = 255;
const uint8_t CHAR_SPACING = 1;
const uint8_t SCROLL_DELAY = 75;

char curMessage[MESG_SIZE];
char newMessage[MESG_SIZE];
bool newMessageAvailable = false;

const char WebPage[] =
"<!DOCTYPE html>" \
"<html>" \
"<head>" \
"<title>Update Display</title>" \
"</head>" \

"<body>" \
"<p><b>MD_MAX72xx set message</b></p>" \

"<form method=\"POST\" action=\"/update\">" \
"<label>Msg:<input type=\"text\" name=\"Message\" maxlength=\"255\"></label><br><br>" \

"<br>" \
"<button type=\"submit\">Send</button>" \
"</form>" \
"</body>" \
"</html>";

String deviceName = "lcddisplay";
String chipId;
char* updInt      = "60";  


const char *err2Str(wl_status_t code)
{
  switch (code)
  {
  case WL_IDLE_STATUS:    return("IDLE");           break; // WiFi is in process of changing between statuses
  case WL_NO_SSID_AVAIL:  return("NO_SSID_AVAIL");  break; // case configured SSID cannot be reached
  case WL_CONNECTED:      return("CONNECTED");      break; // successful connection is established
  case WL_CONNECT_FAILED: return("CONNECT_FAILED"); break; // password is incorrect
  case WL_DISCONNECTED:   return("CONNECT_FAILED"); break; // module is not configured in station mode
  default: return("??");
  }
}

uint8_t htoi(char c)
{
  c = toupper(c);
  if ((c >= '0') && (c <= '9')) return(c - '0');
  if ((c >= 'A') && (c <= 'F')) return(c - 'A' + 0xa);
  return(0);
}

boolean getText(char *szMesg, char *psz, uint8_t len)
{
  boolean isValid = false;  // text received flag
  char *pStart, *pEnd;      // pointer to start and end of text

  // get pointer to the beginning of the text
  pStart = strstr(szMesg, "/&MSG=");

  if (pStart != NULL)
  {
    pStart += 6;  // skip to start of data
    pEnd = strstr(pStart, "/&");

    if (pEnd != NULL)
    {
      while (pStart != pEnd)
      {
        if ((*pStart == '%') && isdigit(*(pStart+1)))
        {
          // replace %xx hex code with the ASCII character
          char c = 0;
          pStart++;
          c += (htoi(*pStart++) << 4);
          c += htoi(*pStart++);
          *psz++ = c;
        }
        else
          *psz++ = *pStart++;
      }

      *psz = '\0'; // terminate the string
      isValid = true;
    }
  }

  return(isValid);
}

void scrollDataSink(uint8_t dev, MD_MAX72XX::transformType_t t, uint8_t col)
// Callback function for data that is being scrolled off the display
{
#if PRINT_CALLBACK
  Serial.print("\n cb ");
  Serial.print(dev);
  Serial.print(' ');
  Serial.print(t);
  Serial.print(' ');
  Serial.println(col);
#endif
}


uint8_t scrollDataSource(uint8_t dev, MD_MAX72XX::transformType_t t)
// Callback function for data that is required for scrolling into the display
{
  static enum { S_IDLE, S_NEXT_CHAR, S_SHOW_CHAR, S_SHOW_SPACE } state = S_IDLE;
  static char *p;
  static uint16_t curLen, showLen;
  static uint8_t  cBuf[8];
  uint8_t colData = 0;

  // finite state machine to control what we do on the callback
  switch (state)
  {
  case S_IDLE: // reset the message pointer and check for new message to load
    PRINTS("\nS_IDLE");
    p = curMessage;      // reset the pointer to start of message
    if (newMessageAvailable)  // there is a new message waiting
    {
      strcpy(curMessage, newMessage); // copy it in
      newMessageAvailable = false;
    }
    state = S_NEXT_CHAR;
    break;

  case S_NEXT_CHAR: // Load the next character from the font table
    PRINTS("\nS_NEXT_CHAR");
    if (*p == '\0')
      state = S_IDLE;
    else
    {
      showLen = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
      curLen = 0;
      state = S_SHOW_CHAR;
    }
    break;

  case S_SHOW_CHAR: // display the next part of the character
    PRINTS("\nS_SHOW_CHAR");
    colData = cBuf[curLen++];
    if (curLen < showLen)
      break;

    // set up the inter character spacing
    showLen = (*p != '\0' ? CHAR_SPACING : (MAX_DEVICES*COL_SIZE)/2);
    curLen = 0;
    state = S_SHOW_SPACE;
    // fall through

  case S_SHOW_SPACE:  // display inter-character spacing (blank column)
    PRINT("\nS_ICSPACE: ", curLen);
    PRINT("/", showLen);
    curLen++;
    if (curLen == showLen)
      state = S_NEXT_CHAR;
    break;

  default:
    state = S_IDLE;
  }

  return(colData);
}

void scrollText(void)
{
  static uint32_t	prevTime = 0;

  // Is it time to scroll the text?
  if (millis() - prevTime >= SCROLL_DELAY)
  {
    mx.transform(MD_MAX72XX::TSL);  // scroll along - the callback will load all the data
    prevTime = millis();      // starting point for next time
  }
}

void handleRoot(AsyncWebServerRequest *request) {
  request->send(200, F("text/html"), WebPage);
}

void handleUpdate(AsyncWebServerRequest *request) {
  AsyncWebParameter* p = request->getParam("Message", true);
  String message = p->value();
  if (message.length() > 0) {
    message.toCharArray(newMessage, MESG_SIZE);
    newMessageAvailable = true;
  }
  request->redirect(F("/"));
}

void setup()
{
#if DEBUG
  Serial.begin(115200);
  PRINTS("\n[MD_MAX72XX WiFi Message Display]\nType a message for the scrolling display from your internet browser");
#endif

#if LED_HEARTBEAT
  pinMode(HB_LED, OUTPUT);
  digitalWrite(HB_LED, LOW);
#endif

 // creat a unique deviceName for classroom situations (deviceName-123)
	
	IAS.preSetDeviceName(deviceName);											  // preset deviceName this is also your MDNS responder: http://webtoggle-123.local
  IAS.preSetAutoUpdate(true);                             // automaticUpdate (true, false)

  // You can configure callback functions that can give feedback to the app user about the current state of the application.
  // In this example we use serial print to demonstrate the call backs. But you could use leds etc.
  IAS.onModeButtonShortPress([]() {
    Serial.println(F(" If mode button is released, I will enter in firmware update mode."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  IAS.onModeButtonLongPress([]() {
    Serial.println(F(" If mode button is released, I will enter in configuration mode."));
    Serial.println(F("*-------------------------------------------------------------------------*"));
  });

  /*
  IAS.onFirstBoot([]() {
    IAS.eraseEEPROM('P');                                 // Optional! What to do with EEPROM on First boot of the app? 'F' Fully erase | 'P' Partial erase
  });
  */
  
  IAS.begin('P');                                            // Run IOTAppStory
  IAS.setCallHomeInterval(atoi(updInt));                  // Call home interval in seconds(disabled by default), 0 = off, use 60s only for development. Please change it to at least 2 hours in production



  // Display initialization
  mx.begin();
  mx.transform(MD_MAX72XX::TFUD);
  mx.setShiftDataInCallback(scrollDataSource);
  mx.setShiftDataOutCallback(scrollDataSink);

  curMessage[0] = newMessage[0] = '\0';

  // Start the server
  server.on("/", handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();
  PRINTS("\nServer started");

  // Set up first message as the IP address
  sprintf(curMessage, "%03d:%03d:%03d:%03d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  PRINT("\nAssigned IP ", curMessage);
}

void loop()
{
#if LED_HEARTBEAT
  static uint32_t timeLast = 0;

  if (millis() - timeLast >= HB_LED_TIME)
  {
    digitalWrite(HB_LED, digitalRead(HB_LED) == LOW ? HIGH : LOW);
    timeLast = millis();
  }
#endif
  IAS.loop();
  //server.handleClient();
  yield();
  scrollText();
  yield(); //allow the cpu to switch to other tasks  
}

