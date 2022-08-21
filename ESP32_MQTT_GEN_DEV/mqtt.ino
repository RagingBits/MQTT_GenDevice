
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Ticker.h>
#include <WiFi.h>
#include <ArduinoMqttClient.h>

#define MQTT_CLIENT_IP_LEN  16U
#define MQTT_CLIENT_THREAD_RATE           100/*milliseconds*/
#define MQTT_CLIENT_RECONNECTION_TIMEOUT  (3000/MQTT_CLIENT_THREAD_RATE) /* 5Seconds */

typedef struct
{
    uint32_t reconnecting:1U;
    uint32_t initialised:1U;
    uint32_t work_run:1U;
    uint32_t unused:29U;
}mqtt_client_flags_t;


/* MQTT Global Constants ------------------------------------------------------ */
//extern const String userID_;

const String COMMAND_RGB_TOPIC_ PROGMEM = "_rgb_set";
const String COMMAND_RELAY_TOPIC_ PROGMEM = "_relay_set";
const String COMMAND_OUTPUT_TOPIC_ PROGMEM = "_outputs_set";
const String COMMAND_INPUT_TOPIC_ PROGMEM =  "_inputs_get";
const String COMMAND_INPUT_TOPIC_REP_ PROGMEM = "_inputs";
const String COMMAND_UART_OUT_TOPIC_ PROGMEM = "_uart_out";
const String COMMAND_UART_IN_TOPIC_ PROGMEM =  "_uart_in";

String COMMAND_RGB_TOPIC = "";
String COMMAND_RELAY_TOPIC = "";
String COMMAND_OUTPUT_TOPIC = "";
String COMMAND_INPUT_TOPIC =  "";
String COMMAND_INPUT_TOPIC_REP = "";
String COMMAND_UART_OUT_TOPIC = "";
String COMMAND_UART_IN_TOPIC =  "";


/* MQTT Global Variables ------------------------------------------------------ */
mqtt_client_flags_t mqtt_client_flags = {.reconnecting = !0U};
uint32_t mqtt_client_reconnection_timeout = 0U;


/* MQTT Clients --------------------------------------------------------------- */
WiFiClient mqtt_wifi_client;
MqttClient mqtt_client(&mqtt_wifi_client);
Ticker mqtt_client_thread;


/* MQTT External Functions ---------------------------------------------------- */
void MQTTConnect(void)
{
    uint8_t counter = 0;
    
    if(0U == mqtt_client_flags.initialised)
    {
        Serial.print("MQTT Thread starting... ");
        mqtt_client_thread.attach_ms(MQTT_CLIENT_THREAD_RATE, &MQTTRunWork);
        mqtt_client_flags.initialised = !0U;
        mqtt_client_flags.reconnecting = !0U;



        COMMAND_RGB_TOPIC  = userID_ + "_rgb_set";
        COMMAND_RELAY_TOPIC  = userID_ + "_relay_set";
        COMMAND_OUTPUT_TOPIC  = userID_ + "_outputs_set";
        COMMAND_INPUT_TOPIC  = userID_ + "_inputs_get";
        COMMAND_INPUT_TOPIC_REP  = userID_ + "_inputs";
        COMMAND_UART_OUT_TOPIC  = userID_ + "_uart_out";
        COMMAND_UART_IN_TOPIC  = userID_ + "_uart_in";


        Serial.println("Done.");
    }

    Serial.println("MQTT New Connection request. "); 
    if(0U == mqtt_client_flags.reconnecting)
    {
        Serial.println("MQTT Disconnect from Broker. ");
        mqtt_client.stop();
        mqtt_client_flags.reconnecting = 1U;
    }
}


bool MQTTIsConnected(void)
{
    return (0U == mqtt_client_flags.reconnecting);
}


void MQTTHandleInputsMessage(uint8_t inputs, uint8_t inputs_mask)
{
    char data_array[4] = {0};
    uint8_t counter = 0;
  
    Serial.println("Inputs:");

    
    
    while(counter < 6)
    {
        if(inputs_mask & 1<<counter)
        {
            data_array[0] = 0x31+counter;  
            data_array[1] = ' ';  
            if(inputs & 1<<counter)
            {
                data_array[2] = '1';  
            }
            else
            {
                data_array[2] = '0';  
            }
            Serial.println(data_array);
            mqtt_client.beginMessage(COMMAND_INPUT_TOPIC_REP);
            mqtt_client.print(data_array);
            mqtt_client.endMessage();
        }
        counter++;
    }  

      
}

/* MQTT Internal Functions ---------------------------------------------------- */

void MQTTRunWork(void)
{
  mqtt_client_flags.work_run = !0U;  
}

void MQTTPerformSubscriptions(void)
{
    Serial.println(" ");
    Serial.println("MQTT Subscribing: ");
    Serial.print("    MQTT Subscribe: ");Serial.println(COMMAND_RGB_TOPIC);
    mqtt_client.subscribe(COMMAND_RGB_TOPIC);
    Serial.print("    MQTT Subscribe: ");Serial.println(COMMAND_RELAY_TOPIC);
    mqtt_client.subscribe(COMMAND_RELAY_TOPIC);
    Serial.print("    MQTT Subscribe: ");Serial.println(COMMAND_OUTPUT_TOPIC);
    mqtt_client.subscribe(COMMAND_OUTPUT_TOPIC);
    Serial.print("    MQTT Subscribe: ");Serial.println(COMMAND_INPUT_TOPIC);
    mqtt_client.subscribe(COMMAND_INPUT_TOPIC);
    Serial.print("    MQTT Subscribe: ");Serial.println(COMMAND_UART_OUT_TOPIC);
    mqtt_client.subscribe(COMMAND_UART_OUT_TOPIC);
}


void MQTTWork(void)
{
    if(0U != mqtt_client_flags.initialised && mqtt_client_flags.work_run != 0U)
    {
        mqtt_client_flags.work_run = 0U;
        if(0U == mqtt_client_flags.reconnecting)
        {
            if(mqtt_client.connected())
            {
                mqtt_client.poll();
            }
            else
            {
                if(0U == mqtt_client_flags.reconnecting)
                {
                    mqtt_client_flags.reconnecting = !0U;
                    MQTTReConnect();
                }
            }
        }
        else
        {
            MQTTReConnect();
        }
    }
   
}


void MQTTReConnect(void)
{
    if(0U != mqtt_client_flags.reconnecting)
    {
        if(0U == mqtt_client_reconnection_timeout)
        {
            if(0U != mqtt_client.connected())
            {
                Serial.println(" ");
                Serial.println("    MQTT Connected. ");
                MQTTPerformSubscriptions();
                mqtt_client_flags.reconnecting = 0U;
                mqtt_client_reconnection_timeout = 0U;
            }
            else
            {
                Serial.println("MQTT Connecting:");

                uint8_t temp_len = MQTT_BROKER_PORT_MAX_LEN;
                char temp_data[MQTT_BROKER_PORT_MAX_LEN+MQTT_BROKER_IP_MAX_LEN] = {0U};
                EepromRead(EEPROM_MQTT_BROKER_PORT, (uint8_t*)temp_data, &temp_len);
                uint16_t mqtt_broker_port_int = atoi(temp_data);

                temp_len = MQTT_BROKER_IP_MAX_LEN;
                EepromRead(EEPROM_MQTT_BROKER_IP, (uint8_t*)temp_data, &temp_len);

                Serial.print("    Broker IP: "); Serial.println(temp_data);
                Serial.print("    Broker port: "); Serial.println(mqtt_broker_port_int);
                Serial.print("    ");
                
                mqtt_client.connect(temp_data, mqtt_broker_port_int);
                
                mqtt_client.onMessage(MQTTHandle);
                mqtt_client_reconnection_timeout = MQTT_CLIENT_RECONNECTION_TIMEOUT;
            }
        }
        else
        {
            Serial.print(".");
            mqtt_client_reconnection_timeout--;
        }
       
    }
}


void MQTTHandle(int message_len)
{
    Serial.println(" ");
    Serial.print("MQTT new message: ");
    String received_topic = mqtt_client.messageTopic();
    
    
    char message_data[100U]={0};
    uint16_t message_data_len = 0;
    while (mqtt_client.available() && message_data_len<100) 
    {
      message_data[message_data_len++] = (char)mqtt_client.read();
    }
    Serial.println((char*)message_data);

    if(COMMAND_RGB_TOPIC == received_topic)
    {
        Serial.println("RGB Color.");
        MQTTHandleRGBMessage(message_data,message_data_len);
    }
    else 
    if(COMMAND_RELAY_TOPIC == received_topic)
    {
        Serial.println("Relay action.");
        MQTTHandleRelayMessage(message_data,message_data_len);
    }
    else
    if(COMMAND_OUTPUT_TOPIC == received_topic)
    {
        Serial.println("IO Outputs.");
        MQTTHandleOutputsMessage(message_data,message_data_len);
      
    }
    else
    if(COMMAND_INPUT_TOPIC == received_topic)
    {
        Serial.println("IO Inputs read.");
        MQTTHandleReadInputsMessage(message_data,message_data_len);
      
    }
    else
    if(COMMAND_UART_OUT_TOPIC == received_topic)
    {
        Serial.println("UART Output.");
        MQTTHandleUartOutputMessage(message_data,message_data_len);
    }
    else
    {
      ;/* Mistake?... */  
    } 
}


void MQTTHandleUartOutputMessage(char *message_data, uint16_t message_len)
{
    //if(IOPinsAltUartIsEn())
    {
        Serial.write(message_data, message_len);
        Serial1.write(message_data, message_len);
    }
}

void MQTTHandleUartInputMessage(uint8_t *message_data, uint16_t data_length)
{
    mqtt_client.beginMessage(COMMAND_UART_IN_TOPIC);

    mqtt_client.write(message_data, data_length);            

    mqtt_client.endMessage();  
}


void MQTTHandleReadInputsMessage(char *message_data, uint16_t message_len)
{
    uint8_t new_pin_state = IOPinsReadInputs();
    MQTTHandleInputsMessage(new_pin_state, 0xff);
}

void MQTTHandleRGBMessage(char *message_data, uint16_t message_len)
{
  uint8_t led_buffer[3] = {0U};
  
  if(message_data[0] == '#' && message_len >= 7)
  {
        //Serial.println("MQTT rgb format. ");  

        char temp_num[3] = {0};
  
        temp_num[0] = message_data[1];
        temp_num[1] = message_data[2];
        
        led_buffer[0] = strtoul((const char*)temp_num, NULL, 16)%256;      
        temp_num[0] = message_data[3];
        temp_num[1] = message_data[4];
        led_buffer[1] = strtoul((const char*)temp_num, NULL, 16)%256;      
        
        temp_num[0] = message_data[5];
        temp_num[1] = message_data[6];
        led_buffer[2] = strtoul((const char*)temp_num, NULL, 16)%256;      

        LedsSetRGB(led_buffer[0],led_buffer[1],led_buffer[2]);

    }
    else
    {

      uint8_t total_counter = 0;
      uint8_t temp_counter = 0;
      uint8_t temp_data[6] = {0};
      uint8_t total_numbers = 0;
      bool run = true;
      bool fail = false;
      while(total_numbers < 3 && !fail)
      {
        temp_counter = 0;
        run = true;
        while(temp_counter <= 5 && run)
        {
          temp_data[temp_counter] = message_data[total_counter++];
          if(' ' == temp_data[temp_counter])
          {
            while(' ' == temp_data[temp_counter])
            {
              temp_data[temp_counter] = message_data[total_counter++];
            }
            temp_data[temp_counter] = 0;
            total_counter--;
            run = false;
          }
          else
          if(temp_data[temp_counter] == 0 && total_numbers < 2)
          {
            Serial.println("End string.");
            run = false;
            fail = true;
          }
          else
          if(temp_data[temp_counter] == 0)
          {
            run = false;
          }
          else
          {
            ;
          }
          temp_counter++;
        }
        
        if(!run)
        {
          //Serial.print("New number ");
          if(temp_data[1] == 'x')
          {
              //Serial.println((char*)temp_data);
              led_buffer[total_numbers++] = strtoul((const char*)temp_data, NULL, 16)%256;                  
              //Serial.println("in hex.");  
            
          }
          else
          {
            /* Decimal number */
            //Serial.println((char*)temp_data);
            led_buffer[total_numbers++] = atoi((const char*)temp_data)%256;
            //Serial.println("in decimal.");  
          }
        }
        else
        {
          fail = true;  
        }
        //Serial.println(total_numbers);  
      }
      //Serial.println("Finished.");  
      
      if(!fail)
      {
        LedsSetRGB(led_buffer[0],led_buffer[1],led_buffer[2]);
      }
    }
}

/* Message format:
Relay 1 - off, Relay 2 - on ->  <space>1<space>0<space>2<space>1<space> 
Relay 1 - on ->  1<space>1  
Relay 2 - of ->  2<space>0
*/
void MQTTHandleRelayMessage(char *message_data, uint16_t message_len)
{
  uint16_t message_counter = message_len;
  uint8_t value = 0;
  uint8_t cycle_status = 0;
  uint8_t relay = 0xFF;
  
  
  while(message_counter--)
  {
    value = *message_data++;
    if(value != ' ')
    {
      switch(cycle_status)  
      {
        case 0:
        {
          if(value == '1')
          {
            relay = RELAY_1;
            cycle_status++;
          }
          else
          if(value == '2')
          {
            relay = RELAY_2;
            cycle_status++;
          }
        }
        break;

        case 1:
        {
          if(value == '0')
          {
            IOPinsSetRelay(relay, false);
          }
          else
          if(value == '1')
          {
            IOPinsSetRelay(relay, true);
          }

          cycle_status = 0;
        }
        break;
      }
    }
  }
  
}

void MQTTHandleOutputsMessage(char *message_data, uint16_t message_len)
{
    uint16_t message_counter = message_len;
    uint8_t value = 0;
    uint8_t cycle_status = 0;
    uint8_t output = 0x00;
    uint8_t mask = 0x00;
    uint8_t indexer = 1;
  
    while(message_counter--)
    {
        value = *message_data++;
        if(value != ' ')
        {
          switch(cycle_status)  
          {
            case 0:
            {
                
                value -= 0x31;
                if(value<6)
                {
                    mask |= (1<<value);
                    indexer = value;
                    cycle_status++;
                    //Serial.print("OUT ");
                    //Serial.print((1<<value));
                    //Serial.print(" as ");
                }
            }
            break;
    
            case 1:
            {
                //Serial.println(value);
                if(value == '0')
                {
                    output &= ~(1<<indexer);
                }
                else
                if(value == '1')
                {
                    output |= (1<<indexer);
                }
                cycle_status = 0;
                
            }
            break;
          }
        }
    }
    Serial.println(output);
    IOPinsWriteOutputs(output, mask);
}
