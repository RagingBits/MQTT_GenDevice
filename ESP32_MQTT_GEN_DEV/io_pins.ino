#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "pins_arduino.h"
#include <Ticker.h>


/* IO_Pins Definitions --------------------------------------------------------- */

#define IO_PINS_CLIENT_THREAD_RATE           100/*milliseconds*/
#define IO_PINS_CLIENT_READ_INPUTS_TIMEOUT   (500/IO_PINS_CLIENT_THREAD_RATE) /* 1Seconds */
#define IO_PINS_IO_BUTTON_HOLD_TIMEOUT       (3000/IO_PINS_CLIENT_THREAD_RATE) /* 1Seconds */

#if (HARDWARE_VERSION == HW_V_1)
const uint8_t io_system_pins[] PROGMEM = {22,21,23, 4,15,2, 18,19, 36,34,32,25,27,12, 39,35,33,26,14,13, 5,0, 1,3, 17,16};
#elif (HARDWARE_VERSION == HW_V_2)
const uint8_t io_system_pins[] PROGMEM = {22,21,23, 4,15,2, 18,19, 33,26,32,25,27,12, 13,14,34,36,35,39, 5,0, 1,3, 17,16}; /* new */
#else
#error  Hardware version is invalid 
#endif

typedef struct
{
    uint32_t initialised:1U;
    uint32_t button_hold:1U;
    uint32_t button_press:1U;
    uint32_t work_run:1U;
    uint32_t inputs:6U;
    uint32_t uart3_enabled:1U;
    uint32_t check_uart3:1U;
    uint32_t unused:20U;
}io_pins_flags_t;

/* IO_Pins Constants ----------------------------------------------------------- */



/* IO_Pins Variables ----------------------------------------------------------- */
static uint8_t input_status = 0;
static uint32_t input_check = 0;
static uint32_t io_pins_check_inputs_timeout = 0;
static uint32_t io_pins_check_button_hold_timeout = 0;
static io_pins_flags_t io_pins_flags = {0U};

/* IO_Pins Clients ------------------------------------------------------------- */
Ticker io_pins_client_thread;

/* IO_Pins External Functions -------------------------------------------------- */
void IOPinsStart(void)
{
  
    Serial.println(" ");
    Serial.print("IO Pins initialising... "); 
    if(0U == io_pins_flags.initialised)
    {
        IOPinsInitialise();
        io_pins_flags.inputs = ~IOPinsReadInputs();
        io_pins_client_thread.attach_ms(IO_PINS_CLIENT_THREAD_RATE, &IOPinsRunWork);
        
        io_pins_flags.initialised = !0U;
    }
    Serial.println("done.");
}

bool IOPinsAltUartIsEn(void)
{
    return (0U != io_pins_flags.uart3_enabled);
}


void IOPinsToggleLEDs(uint8_t led)
{
    if(digitalRead(io_system_pins[led]) != 0)
    {
        IOPinsSetLEDs(led, false);
    }
    else
    {
        IOPinsSetLEDs(led, true);
    }
}

void IOPinsSetLEDs(uint8_t led, bool enabled)
{
    if(WIFI_LED == led)
    {
        //Serial.print("WiFi Led ");
        if(enabled)
        {
            //Serial.println("ON");
            digitalWrite(io_system_pins[led], LOW);
        }
        else
        {
            //Serial.println("OFF");
            digitalWrite(io_system_pins[led], HIGH);
        }
    }  
    else
    if(ERROR_LED == led)
    {
        //Serial.print("Error Led ");
        if(enabled)
        {
            //Serial.println("ON");
            digitalWrite(io_system_pins[led], LOW);
        }
        else
        {
            //Serial.println("OFF");
            digitalWrite(io_system_pins[led], HIGH);
        }
    }
    else
    if(WORK_LED == led)
    {
        //Serial.print("Work Led ");
        if(enabled)
        {
            //Serial.println("ON");
            digitalWrite(io_system_pins[led], LOW);
        }
        else
        {
            //Serial.println("OFF");
            digitalWrite(io_system_pins[led], HIGH);
        }
    }
}


void IOPinsSetSerialPort3(bool enabled)
{
    Serial.println(" ");
    Serial.println("IO Pins UART3: ");
    Serial.print("    Pins set as ");
    
    if(enabled)
    {
        Serial.println(" UART mode.");
        Serial.print("    TX - IO"); Serial.print(io_system_pins[ESP32_UART_TX]);
        Serial.print(" : RX - IO"); Serial.print(io_system_pins[ESP32_UART_RX]);
        
        uint32_t uart3_speed = 0U;
        uint8_t uart3_speed_[UART3_SPEED_MAX_LEN] = {0U};
        uint8_t uart3_speed_len = UART3_SPEED_MAX_LEN;
        EepromRead(EEPROM_UART3_SPEED, (uint8_t *)uart3_speed_, &uart3_speed_len);
        uart3_speed = atoi((char*)uart3_speed_);
        
        Serial.print(" : "); Serial.print(uart3_speed);
        Serial.end();
        Serial1.begin(uart3_speed,SERIAL_8N1, io_system_pins[ESP32_UART_RX], io_system_pins[ESP32_UART_TX]);
        io_pins_flags.uart3_enabled = !0U;
    }
    else
    {
      
        Serial.println(" IO mode.");
        Serial1.end();
        Serial.begin(115200,SERIAL_8N1, io_system_pins[ESP32_UART_RX], io_system_pins[ESP32_UART_TX]);
        //pinMode(io_system_pins[USER_UART3_TX],INPUT_PULLUP);
        //pinMode(io_system_pins[USER_UART3_RX],OUTPUT_OPEN_DRAIN);  
        //digitalWrite(io_system_pins[USER_UART3_TX],HIGH);
        io_pins_flags.uart3_enabled = 0U;
    }


}

void IOPinsSetRelay(uint8_t relay, bool enabled)
{
    Serial.println(" ");
    Serial.println("IO Pins ");
    Serial.print("    Set Relay ");
    
    if(RELAY_1 == relay)
    {
        Serial.print("1 ");
        if(enabled)
        {
            Serial.println("ON");
            digitalWrite(io_system_pins[relay],HIGH);
        }
        else
        {
            Serial.println("OFF");
            digitalWrite(io_system_pins[relay],LOW);
        }
    }  
    else
    if(RELAY_2 == relay)
    {
        Serial.print("2 ");
        if(enabled)
        {
            Serial.println("ON");
            digitalWrite(io_system_pins[relay],HIGH);
        }
        else
        {
            Serial.println("OFF");
            digitalWrite(io_system_pins[relay],LOW);
        }
    }  
}

void IOPinsForceUart3Check(void)
{
    io_pins_flags.check_uart3 = !0U;
}

void IOPinsWork(void)
{
    uint8_t temp_read = 0U;
    
    if(io_pins_flags.work_run)  
    {
        
        if(0U == io_pins_check_inputs_timeout)
        {
            if(MQTTIsConnected())  
            {
                temp_read = IOPinsReadInputs();
                if(io_pins_flags.inputs != temp_read)
                {
                    uint8_t changes = (temp_read^io_pins_flags.inputs);
                    io_pins_flags.inputs = temp_read;
                    MQTTHandleInputsMessage(io_pins_flags.inputs, changes);
                }
            }
            io_pins_check_inputs_timeout = IO_PINS_CLIENT_READ_INPUTS_TIMEOUT;
        }
        else
        {
            io_pins_check_inputs_timeout--;  

            if(0U != io_pins_flags.check_uart3)
            {
                uint8_t uart3_en[UART3_EN_MAX_LEN] = {0U};
                uint8_t uart3_en_len = UART3_EN_MAX_LEN;
                EepromRead(EEPROM_UART3_EN, uart3_en, &uart3_en_len);
              
                if('1' == uart3_en[0U])
                {
                    IOPinsSetSerialPort3(true);
                }
                io_pins_flags.check_uart3 = 0U;
            }
        }


        if(0U == digitalRead(io_system_pins[MEM_ERASE_REQ]))
        {
            io_pins_flags.button_press = !0U;
            if(io_pins_check_button_hold_timeout == IO_PINS_IO_BUTTON_HOLD_TIMEOUT)
            {
                io_pins_flags.button_hold = !0U;
            }
            else
            {
                io_pins_check_button_hold_timeout++;
            }
        }
        else
        {
            io_pins_flags.button_press = 0U;
            io_pins_flags.button_hold = 0U;
            io_pins_check_button_hold_timeout = 0;
        }
        
      
        io_pins_flags.work_run = 0U;  
    }
}


bool IOPinsButtonPress(void)
{
    return (0U != io_pins_flags.button_press);
}


bool IOPinsButtonHold(void)
{
    return (0U != io_pins_flags.button_hold);
}

uint8_t IOPinsReadInputs(void)
{
  uint8_t inputs = 0,counter = 0;
  
  while(counter < 6)
  {
    inputs *= 2;
    inputs += digitalRead(io_system_pins[INPUTS+counter]);
    counter++;
  }

  return inputs;
}

void IOPinsWriteOutputs(uint8_t outputs, uint8_t mask)
{
  uint8_t counter = 0;
  uint8_t indexer = 1;
  
  Serial.println(" ");
  Serial.println("IO Pins ");
  while(counter < 6)
  {
      if(mask & indexer)
      {
          Serial.print("    Set Output ");
          Serial.print(counter+1);
          if(outputs & indexer)
          {
              Serial.println(" ON.");
              digitalWrite(io_system_pins[OUTPUTS+counter],HIGH);
          }
          else
          {
              Serial.println(" OFF.");
              digitalWrite(io_system_pins[OUTPUTS+counter],LOW);
          }
      }
      
      indexer *= 2;
      counter++;
  }
  
}


/* IO_Pins Internal Functions -------------------------------------------------- */

void IOPinsRunWork(void)
{
    io_pins_flags.work_run = !0U;
}


void digitalToggle(uint8_t io)
{
    if(digitalRead(io) != 0)
    {
        digitalWrite(io,LOW);
    }
    else
    {
        digitalWrite(io,HIGH);
    }  
}



void IOPinsInitialise(void)
{
  uint8_t counter = 0;
  Serial.println(" ");
  Serial.println("IO Pins Initialisation");
  Serial.print("    Initialise Relay pins...");

  pinMode(io_system_pins[PWM_R], OUTPUT);
  digitalWrite(io_system_pins[PWM_R],LOW);
  pinMode(io_system_pins[PWM_G], OUTPUT);
  digitalWrite(io_system_pins[PWM_G],LOW);  
  pinMode(io_system_pins[PWM_B], OUTPUT);
  digitalWrite(io_system_pins[PWM_B],LOW);
  
  /* Setup Relays. */
  pinMode(io_system_pins[RELAY_1], OUTPUT);
  digitalWrite(io_system_pins[RELAY_1],LOW);
  pinMode(io_system_pins[RELAY_2], OUTPUT);
  digitalWrite(io_system_pins[RELAY_2],LOW);
  Serial.println(" done.");
  
  /* Adressable leds driver. */    
  Serial.print("    Initialise RGB LED pins...");
  pinMode(io_system_pins[ADDR_LEDS_DRVR_RST], OUTPUT);
  digitalWrite(io_system_pins[ADDR_LEDS_DRVR_RST],LOW);
  Serial.println(" done.");

  /* System UI leds. */
  Serial.print("    Initialise UI LED pins...");
  pinMode(io_system_pins[WIFI_LED], OUTPUT);
  digitalWrite(io_system_pins[WIFI_LED],HIGH);
  pinMode(io_system_pins[WORK_LED], OUTPUT);
  digitalWrite(io_system_pins[WORK_LED],HIGH);
  pinMode(io_system_pins[ERROR_LED], OUTPUT);  
  digitalWrite(io_system_pins[ERROR_LED],HIGH);
  Serial.println(" done.");

  
  /* System UI inputs. */
  Serial.print("    Initialise UI button pin...");
  pinMode(io_system_pins[MEM_ERASE_REQ], INPUT_PULLUP);
  Serial.println(" done.");
  
  /* Setup Inputs and outputs. */
  Serial.print("    Initialise IO pins...");
  while(counter < 6)
  {
    pinMode(io_system_pins[INPUTS+counter], INPUT_PULLUP);
    pinMode(io_system_pins[OUTPUTS+counter], OUTPUT_OPEN_DRAIN);  
    digitalWrite(io_system_pins[OUTPUTS+counter],HIGH);
    counter++;
  }
  Serial.println(" done.");

}
