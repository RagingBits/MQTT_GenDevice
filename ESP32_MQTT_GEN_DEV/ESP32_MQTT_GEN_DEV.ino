/*
    Read me:

All the libraries used are supplied by arduino packages. 
In order to correctly compile this code, these must be included.

Arduino MQTT library.
SPIFFS and EEPROM
WiFi, WebServer and WebClient.

*/

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Ticker.h>


/* Main Definitions ------------------------------------------------------ */
#define FAST_LED_BLINK       200 /* milliseconds */
#define SLOW_LED_BLINK       (2)/* x FAST_LED_BLINK*/

#define MAIN_LOOP_SLEEP      10 /* milliseconds */

#define MAIN_MQTT_CONNECTION_TIMEOUT  (10000/MAIN_LOOP_SLEEP) /* milliseconds */
#define MAIN_WIFI_CONNECTION_TIMEOUT  (15000/MAIN_LOOP_SLEEP) /* milliseconds */

/* SYSTEM Pins definitions -----------------------------------------------*/
#define WIFI_LED            0
#define ERROR_LED           1
#define WORK_LED            2
#define PWM_R               3
#define PWM_G               4
#define PWM_B               5
#define RELAY_1             6
#define RELAY_2             7
#define OUTPUTS             8
#define INPUTS              14
#define ADDR_LEDS_DRVR_RST  20
#define MEM_ERASE_REQ       21
#define ESP32_UART_TX       22
#define ESP32_UART_RX       23
#define ADDR_LEDS_DRVR_TX   24
#define ADDR_LEDS_DRVR_RX   25


/* Bosrd Leds different blink options ------------------------------------*/
#define LED_OFF          0U
#define LED_ON           1U
#define LED_SLOW_BLINK   2U
#define LED_SLOW_nBLINK  3U
#define LED_FAST_BLINK   4U
#define LED_FAST_nBLINK  5U


typedef struct
{
  uint32_t error_led: 3U;
  uint32_t work_led: 3U;
  uint32_t wifi_led: 3U;
  uint32_t main_loop_status: 4U;
  uint32_t blink_delay: 8U;
  uint32_t unused: 11U;
} main_flags_t;


/* Main Constants -------------------------------------------------------- */
extern const String *SERIAL_NUMBER_;
extern String userID_;

const char* userID PROGMEM = userID_.c_str();

/* Main Variables -------------------------------------------------------- */
static main_flags_t main_flags = {0U};


/* Main  Clients --------------------------------------------------------- */
Ticker main_client_thread;

/* Main Functions -------------------------------------------------------- */

void MainLedsBlinkWork(void)
{
  static bool led_state = false;

  if (0U == main_flags.blink_delay)
  {
    main_flags.blink_delay = SLOW_LED_BLINK;

    if (LED_SLOW_BLINK == main_flags.error_led)
    {
      IOPinsSetLEDs(ERROR_LED, led_state);
    }
    else if (LED_SLOW_nBLINK == main_flags.error_led)
    {
      IOPinsSetLEDs(ERROR_LED, !led_state);
    }

    if (LED_SLOW_BLINK == main_flags.work_led)
    {
      IOPinsSetLEDs(WORK_LED, led_state);
    }
    else if (LED_SLOW_nBLINK == main_flags.work_led)
    {
      IOPinsSetLEDs(WORK_LED, !led_state);
    }

    if (LED_SLOW_BLINK == main_flags.wifi_led)
    {
      IOPinsSetLEDs(WIFI_LED, led_state);
    }
    else if (LED_SLOW_nBLINK == main_flags.wifi_led)
    {
      IOPinsSetLEDs(WIFI_LED, !led_state);
    }
  }
  else
  {
    main_flags.blink_delay--;
  }

  if (LED_FAST_BLINK == main_flags.error_led)
  {
    IOPinsSetLEDs(ERROR_LED, led_state);
  }
  else if (LED_FAST_nBLINK == main_flags.error_led)
  {
    IOPinsSetLEDs(ERROR_LED, !led_state);
  }
  else if (LED_ON == main_flags.error_led)
  {
    IOPinsSetLEDs(ERROR_LED, true);
  }
  else if (LED_OFF == main_flags.error_led)
  {
    IOPinsSetLEDs(ERROR_LED, false);
  }

  if (LED_FAST_BLINK == main_flags.work_led)
  {
    IOPinsSetLEDs(WORK_LED, led_state);
  }
  else if (LED_FAST_nBLINK == main_flags.work_led)
  {
    IOPinsSetLEDs(WORK_LED, !led_state);
  }
  else if (LED_ON == main_flags.work_led)
  {
    IOPinsSetLEDs(WORK_LED, true);
  }
  else if (LED_OFF == main_flags.work_led)
  {
    IOPinsSetLEDs(WORK_LED, false);
  }

  if (LED_FAST_BLINK == main_flags.wifi_led)
  {
    IOPinsSetLEDs(WIFI_LED, led_state);
  }
  else if (LED_FAST_nBLINK == main_flags.wifi_led)
  {
    IOPinsSetLEDs(WIFI_LED, !led_state);
  }
  else if (LED_ON == main_flags.wifi_led)
  {
    IOPinsSetLEDs(WIFI_LED, true);
  }
  else if (LED_OFF == main_flags.wifi_led)
  {
    IOPinsSetLEDs(WIFI_LED, false);
  }

  led_state = !led_state;
}

void Reset(void)
{
  Serial.println("Restarting...");
  delay(1000);
  ESP.restart();
  while (1);
}


String GetSetNewSerialNumber(void)
{
    uint32_t timeout = 500U;  
    uint8_t data_in_counter = 0U;
    char read_data[50U] = {0U};
    String return_value = "";
    while(timeout--)
    {
        
        delay(10);
        int16_t result = Serial.read();
        
        if(result != -1)
        {
            read_data[data_in_counter++] = result;
        }
    }


    if(data_in_counter == 10)
    {
        /* Serial number submited. */  
        //if(0 !=  atoi(read_data))
        {
            return_value = String(read_data);
        }
    }

    return return_value;
}

void setup(void)
{
  uint32_t timeout = 5000;
/*
  uint8_t counter = 0U;

  while(counter < 10)
  {
      userID_[counter +(sizeof(userID_)-10)] = (*SERIAL_NUMBER_)[counter];
  }
  */
    
  IOPinsStart();
  
  Serial.begin(115200);
  Serial.println(" ");
  Serial.println(" ");
  Serial.println("---------------------------------------------");
  Serial.println("|=--< Raging Bits - MQTT Generic Device >--=|");
  Serial.println("---------------------------------------------");
  Serial.println(" ");
  Serial.println("System Starting... ");

  String new_serial = GetSetNewSerialNumber();
  
  main_flags.work_led = LED_FAST_BLINK;
  main_client_thread.attach_ms(FAST_LED_BLINK, MainLedsBlinkWork);

  EepromStart();

  EepromSetLoadSerialNumber(new_serial);
  
  WifiSetHotspot(true);

  LedsReset();
  LedsStart();

  while ((0U != timeout) && !LedsReady())
  {
    LedsWork();
    delay(10);
    timeout--;
  }

  if (0 == timeout)
  {
    Reset();
  }

  AlternateUartStart();
  Serial.println(" ");
  Serial.println("System Start up done.");
}



void loop(void)
{
  static uint32_t connection_timeout = 0;

  MQTTWork();
  WifiWork();
  LedsWork();
  IOPinsWork();
  AlternateUartWork();

  delay(MAIN_LOOP_SLEEP);

  if (IOPinsButtonHold())
  {
    main_flags.error_led = LED_ON;
    EepromClear();
    delay(1000);
    while (IOPinsButtonHold()) {
      IOPinsWork();
    }
    Reset();
  }

  switch (main_flags.main_loop_status)
  {
    case 0U:
      {
        main_flags.error_led = LED_FAST_nBLINK;
        WifiStart();
        main_flags.main_loop_status++;
        connection_timeout = MAIN_WIFI_CONNECTION_TIMEOUT;
      }
      break;

    case 1U:
      {
        if (WifiConnected())
        {
          connection_timeout = MAIN_MQTT_CONNECTION_TIMEOUT;
          main_flags.work_led = LED_SLOW_BLINK;
          main_flags.wifi_led = LED_SLOW_nBLINK;
          main_flags.error_led = LED_OFF;
          MQTTConnect();
          main_flags.main_loop_status++;
        }
        else
        {
          if (0U == connection_timeout)
          {
            main_flags.main_loop_status = 0U;
          }
          else
          {
            connection_timeout--;
          }
        }
      }
      break;

    case 2U:
      {
        if (MQTTIsConnected())
        {
          main_flags.wifi_led = LED_ON;
          main_flags.main_loop_status++;
        }
        else
        {
          if (0U == connection_timeout)
          {
            main_flags.main_loop_status = 0U;
            main_flags.work_led = LED_FAST_BLINK;
            //EepromClear();
          }
          else
          {
            connection_timeout--;
          }
        }

      }
      break;

    case 3U:
      {
        if (!MQTTIsConnected())
        {
          main_flags.work_led = LED_FAST_BLINK;
          main_flags.wifi_led = LED_OFF;
          main_flags.main_loop_status = 0U;
        }
      }
      break;
  }

}
