
#include <driver/ledc.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Ticker.h>


/* LEDs Constants ------------------------------------------------------------ */
const uint8_t   channelR          PROGMEM = 0;
const uint8_t   channelG          PROGMEM = 1;
const uint8_t   channelB          PROGMEM = 2;

const uint8_t   channelR_adjust   PROGMEM = 0;
const uint8_t   channelG_adjust   PROGMEM = 0;
const uint8_t   channelB_adjust   PROGMEM = 0;

const uint8_t   numberOfBits      PROGMEM = 8;
const uint32_t  frequency         PROGMEM = 200;



/* LEDs Definitions ----------------------------------------------------------- */
#define LEDS_DELAY_RESET        10
#define LEDS_STRIP_LENGTH       300
#define SEND_LEDS_STATUS_STOP   4
#define SEND_LEDS_STATUS_START  5

#define LEDS_CLIENT_THREAD_RATE           100/*milliseconds*/
#define LEDS_CLIENT_RECONNECTION_TIMEOUT  (3000/LEDS_CLIENT_THREAD_RATE) /* 5Seconds */

typedef struct
{
    uint32_t initialised:1U;
    uint32_t work_run:1U;
    uint32_t addr_leds_drivr_ready:1U;
    uint32_t addr_leds_drivr_update:1U;
    uint32_t addr_leds_drivr_status:6U;
    uint32_t unused:24U;
}leds_client_flags_t;


/* LEDs Global Variables ------------------------------------------------------ */
uint8_t  leds_rgb_buffer[3U];
leds_client_flags_t leds_client_flags = {0U};

/* LEDs Clients --------------------------------------------------------------- */
Ticker leds_client_thread;


/* LEDs External Functions ---------------------------------------------------- */
void LedsReset(void)
{
    Serial.println(" ");
    Serial.print("LEDs reseting... ");
    
    if(0U != leds_client_flags.initialised)
    {
        leds_client_flags.addr_leds_drivr_ready = 0U;
        leds_client_flags.addr_leds_drivr_update = 0U;
        leds_client_flags.addr_leds_drivr_status = 0U;
    
        ledcWrite(channelR, 0);
        ledcWrite(channelG, 0);
        ledcWrite(channelB, 0);
    }   
    
    digitalWrite(io_system_pins[ADDR_LEDS_DRVR_RST],LOW);
    delay(500);
    digitalWrite(io_system_pins[ADDR_LEDS_DRVR_RST],HIGH);  
    delay(100);  
    
    Serial.println("done.");
}

void LedsStart(void)
{
    uint8_t temp_leds_rgb_format[3U] = {0U};
    uint8_t temp_var = 3U;
    
    if(0U == leds_client_flags.initialised)
    {
        Serial.println(" ");
        Serial.println("LEDs Starting.");
        Serial.print("    Starting serial port...");
        
        Serial2.begin(500000,SERIAL_8N1, io_system_pins[ADDR_LEDS_DRVR_RX], io_system_pins[ADDR_LEDS_DRVR_TX]);
        
        Serial.println("done.");
        Serial.print("    Starting PWM channels...");
        ledcSetup(channelR, frequency, numberOfBits);
        ledcSetup(channelG, frequency, numberOfBits);
        ledcSetup(channelB, frequency, numberOfBits);
        
        ledcAttachPin(io_system_pins[PWM_R], channelR);
        ledcAttachPin(io_system_pins[PWM_G], channelG);
        ledcAttachPin(io_system_pins[PWM_B], channelB);
        
        ledcWrite(channelR, 0);
        ledcWrite(channelG, 0);
        ledcWrite(channelB, 0);
        
        
        EepromRead(EEPROM_RGB_FORMAT, temp_leds_rgb_format, &temp_var);
    
        if(temp_leds_rgb_format[0] != 'R' && temp_leds_rgb_format[0] != 'G' && temp_leds_rgb_format[0] != 'B')
        {
            temp_leds_rgb_format[0] = 'R';
            temp_leds_rgb_format[1] = 'G';
            temp_leds_rgb_format[2] = 'B';
            EepromWrite(EEPROM_RGB_FORMAT, temp_leds_rgb_format, 3);
        }
        Serial.println(" done.");
        Serial.print("    Starting LEDs thread...");
        leds_client_thread.attach_ms(LEDS_CLIENT_THREAD_RATE, LedsWorkRun);
        Serial.println(" done.");
        leds_client_flags.initialised = !0U;
    }    
}

void LedsSetRGB(uint8_t r, uint8_t g, uint8_t b)
{
  if(0U != leds_client_flags.initialised)
  {
      Serial.println("LEDs new setting: ");

      ledcWrite(channelR, r);
      ledcWrite(channelG, g);
      ledcWrite(channelB, b);
      
      Serial.print("    RED: ");
      Serial.println(r);
      Serial.print("    GREEN: ");
      Serial.println(g);
      Serial.print("    BLUE: ");
      Serial.println(b);
      
      leds_rgb_buffer[0U] = r;
      leds_rgb_buffer[1U] = g;
      leds_rgb_buffer[2U] = b;
      LedsRGBReformat(leds_rgb_buffer);
            
      leds_client_flags.addr_leds_drivr_update = !0U;
  }
}

void LedsWork(void)
{
    if((0U != leds_client_flags.initialised) && (0U != leds_client_flags.work_run))
    {
        if((0U == leds_client_flags.addr_leds_drivr_ready) || (0U != leds_client_flags.addr_leds_drivr_update))
        {
            LedsAddressbleDriver();
        }
        
        leds_client_flags.work_run = 0U;
    }
}

bool LedsReady(void)
{
    return (0U != leds_client_flags.addr_leds_drivr_ready);  
}


/* LEDs Internal Functions ---------------------------------------------------- */
void LedsRGBReformat(uint8_t *rgb)
{
  uint8_t temp_leds_rgb_format[3];
  
  uint8_t temp_rgb[3] = {rgb[0],rgb[1],rgb[2]};
  uint8_t rgb_data_len = 3;
  
  EepromRead(EEPROM_RGB_FORMAT, temp_leds_rgb_format, &rgb_data_len);

  switch(temp_leds_rgb_format[0])
  {
    case 'G':
    {
      rgb[0] = temp_rgb[1];
    }
    break; 
    case 'B':
    {
      rgb[0] = temp_rgb[2];
    }
    break; 
    default:
    break;
  }
  
  switch(temp_leds_rgb_format [1])
  {
    case 'R':
    {
      rgb[1] = temp_rgb[0];
    }
    break; 
    case 'B':
    {
      rgb[1] = temp_rgb[2];
    }
    break; 
    default:
    break;
  }

  switch(temp_leds_rgb_format[2])
  {
    case 'R':
    {
      rgb[2] = temp_rgb[0];
    }
    break; 
    case 'G':
    {
      rgb[2] = temp_rgb[1];
    }
    break; 
    default:
    break;
  }
}


void LedsWorkRun(void)
{
    leds_client_flags.work_run = !0U;
}


void LedsAddressbleDriver(void)
{  
    static uint32_t leds_delay = 2U;
  
    switch(leds_client_flags.addr_leds_drivr_status)
    {
        case 0:
        {
            Serial.println(" ");
            Serial.println("Start Handshake with Addressable LEDs driver.");
            Serial.print("    Starting auto baud...");
            leds_client_flags.addr_leds_drivr_status++;
        }
        break;
      
        case 1:
        {
          
          if(Serial2.read() == 'X')
          {
            /* Empty buffer. */
            while(Serial2.read() != -1){;}
            Serial.println(" done.");
            Serial.print("    Setting LEDs strip length...");
            
            Serial2.print("L");
            uint8_t temp[2] = {(3U*LEDS_STRIP_LENGTH)/256,(3U*LEDS_STRIP_LENGTH)%256}; /* value '900' in 2 bytes 3 132, msb first. */
            Serial2.write(temp,2);
            Serial2.print("\n");
            
            leds_client_flags.addr_leds_drivr_status++;
          }
          else
          {
            Serial.print(".");
            Serial2.write("XXX",3);
          }
        }
        break;
  
        case 2:
        {
          if(Serial2.read() == 'Y')
          {
              Serial.println(" done.");
              Serial.println("    Addressable LEDs driver is ready.");
              leds_client_flags.addr_leds_drivr_ready = !0U;
              leds_client_flags.addr_leds_drivr_status++;
          }
          else
          {
              Serial.print(".");
          }
        }
        break;
  
        case 3:
        {
            if(Serial2.read() == 'R')
            {        
                uint32_t counter = 0;
                while(counter++ < LEDS_STRIP_LENGTH)
                {
                    Serial2.write(leds_rgb_buffer,3);
                }
                leds_client_flags.addr_leds_drivr_status++;
                leds_delay = LEDS_DELAY_RESET;
            }
            else
            {
                if(leds_delay > 0)
                {
                    leds_delay--;
                }
                else
                {
                    /* Fill in with nulls. */
                    Serial2.write("\0",1);
                }
            }
        }
        break;
      
        case 4:
        {
            /* All data sent, wait for Yes signal from driver. */
            if(Serial2.read() == 'Y')
            {
                leds_client_flags.addr_leds_drivr_status--;
                leds_delay = LEDS_DELAY_RESET;
            }
            else
            {
                if(leds_delay > 0)
                {
                    leds_delay--;
                }
                else
                {
                    /* Fill in with nulls. */
                    Serial2.write("\0\0\0\0",4);
                }
            }
          
        }
        break;
  
        default:
        {}
        break;
    }
}
