

#include <SPIFFS.h>
#include "EEPROM.h"



/* EPROM definitions and variables -------------------------------------------- */
#define SERIAL_NUM_ADDR             0
#define SERIAL_NUM_MAX_LEN          (11)

#define SSID_ADDR                   (SERIAL_NUM_ADDR+SERIAL_NUM_MAX_LEN)
#define SSID_MAX_LEN                (50)

#define PASS_ADDR                   (SSID_ADDR+SSID_MAX_LEN)
#define PASS_MAX_LEN                (50)

#define MQTT_BROKER_IP_ADDR         (PASS_ADDR+PASS_MAX_LEN)
#define MQTT_BROKER_IP_MAX_LEN      (20)

#define MQTT_BROKER_PORT_ADDR       (MQTT_BROKER_IP_ADDR+MQTT_BROKER_IP_MAX_LEN)
#define MQTT_BROKER_PORT_MAX_LEN    (10)

#define RGB_FORMAT_ADDR             (MQTT_BROKER_PORT_ADDR+MQTT_BROKER_PORT_MAX_LEN)
#define RGB_FORMAT_MAX_LEN          (4)

#define UART3_SPEED_ADDR            (RGB_FORMAT_ADDR+RGB_FORMAT_MAX_LEN)
#define UART3_SPEED_MAX_LEN         (7)

#define UART3_EN_ADDR               (UART3_SPEED_ADDR+UART3_SPEED_MAX_LEN)
#define UART3_EN_MAX_LEN            (2)

#define EEPROM_LEN                  (UART3_EN_ADDR+UART3_EN_MAX_LEN)


typedef struct
{
    uint32_t initialised:1U;
    uint32_t unused:31U;
}eeprom_client_flags_t;


#define EEPROM_SERIAL_NUM         0
#define EEPROM_SSID               1
#define EEPROM_PARAPHRASE         2
#define EEPROM_MQTT_BROKER_IP     3
#define EEPROM_MQTT_BROKER_PORT   4
#define EEPROM_RGB_FORMAT         5
#define EEPROM_UART3_SPEED        6
#define EEPROM_UART3_EN           7
#define EEPROM_TOTAL              8


/* EEPROM Global Variables ---------------------------------------------------- */
uint8_t eeprom_ssid[SSID_MAX_LEN];
uint8_t eeprom_paraphrase[PASS_MAX_LEN];
uint8_t eeprom_mqtt_broker_ip[MQTT_BROKER_IP_MAX_LEN];
uint8_t eeprom_mqtt_broker_port[MQTT_BROKER_PORT_MAX_LEN];
uint8_t eeprom_rgb_format[RGB_FORMAT_MAX_LEN];
uint8_t eeprom_uart3_speed[UART3_SPEED_MAX_LEN];
uint8_t eeprom_uart3_en[UART3_EN_MAX_LEN];
uint8_t eeprom_serial_num[SERIAL_NUM_MAX_LEN];
eeprom_client_flags_t eeprom_client_flags = {0U};


/* EEPROM Global Constants ---------------------------------------------------- */
const uint8_t eeprom_lengths[EEPROM_TOTAL] = 
{
    SERIAL_NUM_MAX_LEN,
    SSID_MAX_LEN,
    PASS_MAX_LEN,
    MQTT_BROKER_IP_MAX_LEN,
    MQTT_BROKER_PORT_MAX_LEN,
    RGB_FORMAT_MAX_LEN,
    UART3_SPEED_MAX_LEN,
    UART3_EN_MAX_LEN
};

const uint16_t eeprom_addresses[EEPROM_TOTAL] = 
{
    SERIAL_NUM_ADDR,
    SSID_ADDR,
    PASS_ADDR,
    MQTT_BROKER_IP_ADDR,
    MQTT_BROKER_PORT_ADDR,
    RGB_FORMAT_ADDR,
    UART3_SPEED_ADDR,
    UART3_EN_ADDR    
};


uint8_t *const eeprom_variables[EEPROM_TOTAL] = 
{
    &eeprom_serial_num[0U],
    &eeprom_ssid[0U],
    &eeprom_paraphrase[0U],
    &eeprom_mqtt_broker_ip[0U],
    &eeprom_mqtt_broker_port[0U],
    &eeprom_rgb_format[0U],
    &eeprom_uart3_speed[0U],
    &eeprom_uart3_en[0U]
};

uint8_t const eeprom_name[EEPROM_TOTAL][24] = 
{
    "Serial Number",
    "SSID",
    "Paraphrase",
    "Broker IP",
    "Broker Port",
    "RGB Format",
    "Alt UART Speed",
    "Alt UART Enabled"    
};

bool string_compare(char *a, char *b, uint32_t len)
{
    bool equal = false;
    uint32_t counter = len;
    
    while((0U != *a) && (0U != *b) && (*a == *b) && (counter > 0))
    {
        counter--;
        a++;
        b++;
    }  
  
    if((0U == counter) && (0U != len))
    {
        equal = true;
    }
    
    return equal;
}

/* EEPROM External Functions --------------------------------------------------- */

void EepromSetLoadSerialNumber(String new_serial)
{
    uint8_t SerialNumber[50];
    uint8_t SerialNumberLen = 50;
    uint8_t NewSerialNumberLen = 0;
    EepromRead(EEPROM_SERIAL_NUM, SerialNumber, &SerialNumberLen);
  
    String serialNumberString = String((char*)SerialNumber);
  
    if(serialNumberString == "")
    {
        serialNumberString = "0000000000";
    }
    
    if(new_serial != "")
    {
        NewSerialNumberLen = SerialNumberLen;
        new_serial.toCharArray((char*)SerialNumber, NewSerialNumberLen);
        EepromWrite(EEPROM_SERIAL_NUM, SerialNumber, NewSerialNumberLen);
        
        EepromStart();
    }
  
    NewSerialNumberLen = SerialNumberLen;
    uint8_t indexer = userID_.length() - NewSerialNumberLen + 1;/* 0 terminator for .length is not accounted. */
    //Serial.println("Serial");
    while(NewSerialNumberLen--)
    {
        userID_[indexer+NewSerialNumberLen] = SerialNumber[NewSerialNumberLen];
        //Serial.print(SerialNumber[NewSerialNumberLen]);    
    }
    //Serial.println(" ");
}

void EepromStart(void)
{
    uint8_t counter = 0U;
  
    Serial.println(" ");
    Serial.println("EEPROM starting "); 
    if(0U == eeprom_client_flags.initialised)
    {
        if(!EEPROM.begin(EEPROM_LEN))
        {
          Serial.println("failed!!!");  
        }
        eeprom_client_flags.initialised = !0U;
    }
    
    if(0U != eeprom_client_flags.initialised)
    {
    
        Serial.println("    Loading data... "); 
        while(counter < EEPROM_TOTAL)
        {
            EEPROM.readBytes(eeprom_addresses[counter],(void*)eeprom_variables[counter],(size_t)eeprom_lengths[counter]);
            Serial.print("    "); Serial.print((char*)eeprom_name[counter]);  Serial.print(" : "); Serial.println((char*)eeprom_variables[counter]);
            counter++;
        }

        Serial.println("done."); 
        IOPinsForceUart3Check();
    } 
    
}

void EepromClear(void)
{
    uint8_t counter = 1U;

    Serial.println(" ");
    Serial.println("EEPROM erase request.");
    Serial.print("    EEPROM erasing...");

    while(counter < EEPROM_TOTAL)
    {
        
        uint16_t counter2 = eeprom_lengths[counter];
        while(0U != counter2)
        {
            counter2--;
            eeprom_variables[counter][counter2] = 0U;
        }

        EEPROM.writeBytes(eeprom_addresses[counter],(void*)eeprom_variables[counter],(size_t)eeprom_lengths[counter]); 
        counter++;
    }
    
    EEPROM.commit();
    
    Serial.println(" done.");
  
}

void EepromRead(uint8_t data_type, uint8_t *data_in, uint8_t *data_length)
{
    if(0U != eeprom_client_flags.initialised)
    {
        uint8_t counter = *data_length;

        if(counter > eeprom_lengths[data_type])
        {
            counter = *data_length = eeprom_lengths[data_type];
        }
        
        while(0U != counter)
        {
            counter--;
            data_in[counter] = eeprom_variables[data_type][counter];
        }
    }
}

void EepromWrite(uint8_t data_type, uint8_t *data_in, uint8_t data_length)
{
    if(0U != eeprom_client_flags.initialised)
    {
      Serial.println(" "); 
      Serial.println("EEPROM writting "); 
      uint8_t counter = eeprom_lengths[data_type];
      uint8_t counter2 = 0U;
      
      bool result = string_compare((char*)data_in,(char*)eeprom_variables[data_type],eeprom_lengths[data_type]);   
      
      if(!result)
      {
          while(0U != counter)
          {
              counter --;
              eeprom_variables[data_type][counter] = 0U;
          }
      
          counter = data_length;
          if(counter > eeprom_lengths[data_type])
          {
              counter = eeprom_lengths[data_type];
          }
          counter2 = counter;
          
          while(0U != counter)
          {
              counter--;
              eeprom_variables[data_type][counter] = data_in[counter];
          }
          
          Serial.print("    "); Serial.print((char*)eeprom_name[data_type]);  Serial.print(" : "); Serial.println((char*)eeprom_variables[data_type]);
          EEPROM.writeBytes(eeprom_addresses[data_type],(void*)eeprom_variables[data_type], (size_t)counter2); 
          EEPROM.commit();
      }
      else
      {
          Serial.println("    Already the same value.");
      }
    }
}
