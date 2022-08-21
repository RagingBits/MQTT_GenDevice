
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Ticker.h>
#include <WiFi.h>
#include <WebServer.h>


/* WIFI Definitions     ----------------------------------------------------- */

#define WIFI_THREAD_RATE  100 /*milliseconds*/
#define WIFI_CLIENT_RECONNECTION_TIMEOUT  (5000/WIFI_THREAD_RATE) /* 5Seconds */
#define WIFI_HOTSPOT_TIMEOUT              (10000/WIFI_THREAD_RATE) /* 5Seconds */
#define WIFI_SSID_LEN     100
#define WIFI_PARAPH_LEN   100


typedef struct
{
    uint32_t reconnecting:1U;
    uint32_t initialised:1U;
    uint32_t work_run:1U;
    uint32_t hotspot_running:1U;
    uint32_t webpage_to_show:2U;
    uint32_t unused:26U;
}wifi_client_flags_t;

/* WEB pages user interface definitions ----------------------------------------*/
const char* request_webpage =  "<!DOCTYPE html>\r\n\
<html>\r\n\
<body>\r\n\
<img src=\"data:image/jpeg;base64,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\" />\
<h1>Raging Bits\r\nMagic lights.</h1>\r\n\
<h2>WIFI connection information</h2>\r\n\
<form action=\"/action_page.php\" id=\"form1\">\r\n\
<label for=\"ssid\">Wifi SSID:</label>\r\n\
<input type=\"text\" id=\"ssid\" name=\"ssid\" maxlength=\"50\" size=\"50\"><br><br>\r\n\
<label for=\"password\">Password:</label>\r\n\
<input type=\"text\" id=\"password\" name=\"password\" maxlength=\"50\" size=\"50\"><br><br>\r\n\
<label for=\"broker_ip\">MQTT broker IP:</label>\r\n\
<input type=\"text\" id=\"broker_ip\" name=\"broker_ip\" maxlength=\"15\" size=\"15\"><br><br>\r\n\
<label for=\"broker_port\">MQTT broker port:</label>\r\n\
<input type=\"text\" id=\"broker_port\" name=\"broker_port\" maxlength=\"5\" size=\"5\"><br><br>\r\n\
<label for=\"rgb_format\">RGB Format (RGB GRB BRG...):</label>\r\n\
<input type=\"text\" id=\"rgb_format\" name=\"rgb_format\"  maxlength=\"3\" size=\"3\" value=\"RGB\"><br><br>\r\n\
<label for=\"uart3_enabled\">Use Alternate Uart</label>\r\n\
<input type=\"checkbox\" checked=\"\" value=0 name=\"uart3_enabled\" onclick=\"EnableDisableUartSpeedBox(this)\"><br><br>\r\n\
<label for=\"uart3_speed\">Alternate Uart speed:</label>\r\n\
<input type=\"text\" id=\"uart3_speed\" name=\"uart3_speed\" value=\"9600\"  maxlength=\"6\" size=\"6\"><br><br>\r\n\
<input type=\"submit\" value=\"[  --  Connect  --  ]\">\r\n\
</form>\r\n\
<script type=\"text/javascript\">\
function EnableDisableUartSpeedBox(checkbox_to_value) {\
    var uart3_speedNumber = document.getElementById(\"uart3_speed\");\
    uart3_speedNumber.disabled = checkbox_to_value.checked ? false : true;\
    if (!uart3_speedNumber.disabled) {\
        uart3_speedNumber.value = '9600';\
        uart3_speedNumber.focus();\
    }\
    else\
    {\
        uart3_speedNumber.value = \"Disable\";\
    }\
}\
</script>\
</body>\r\n\
</html>\r\n";

const char* wait_webpage_wifi =  "<!DOCTYPE html>\r\n\
<html lang=\"en\">\r\n\
<body>\r\n\
<html>\
<head>\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
<style>\
.spinner {\
  Margin:50px;\
  border: 0px solid #000000;\
  border-radius: 50%;\
  border-top: 3px solid #000000;\
  width: 80px;\
  height: 80px;\
  -webkit-animation: spin 2s linear infinite; \
  animation: spin 2s linear infinite;\
}\
@-webkit-keyframes spin {\
  0% { -webkit-transform: rotate(0deg); }\
  50% { -webkit-transform: rotate(720deg); }\
  100% { -webkit-transform: rotate(0deg); }\
}\
@keyframes spin {\
  0% { transform: rotate(0deg); }\
  50% { transform: rotate(720deg); }\
  100% { transform: rotate(0deg); }\
</style>\
</head>\
<h1>Connecting...</h1>\r\n\
<div class=\"spinner\"><h2>Weee!</h2></div>\
<h1>Please wait, connecting to WiFi...</h1>\r\n\
<meta http-equiv=\"refresh\" content=\"2\" >\
<h1> </h1>\r\n\
<p>RagingBits.</p>\r\n\
</body>\r\n\
</html>";

const char* wait_webpage_mqtt =  "<!DOCTYPE html>\r\n\
<html lang=\"en\">\r\n\
<body>\r\n\
<html>\
<head>\
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
<style>\
.spinner {\
  Margin:50px;\
  border: 0px solid #000000;\
  border-radius: 50\%;\
  border-top: 3px solid #000000;\
  width: 80px;\
  height: 80px;\
  -webkit-animation: spin 2s linear infinite; \
  animation: spin 2s linear infinite;\
}\
@-webkit-keyframes spin {\
  0% { -webkit-transform: rotate(0deg); }\
  50% { -webkit-transform: rotate(720deg); }\
  100% { -webkit-transform: rotate(0deg); }\
}\
@keyframes spin {\
  0% { transform: rotate(0deg); }\
  50% { transform: rotate(720deg); }\
  100% { transform: rotate(0deg); }\
</style>\
</head>\
<h1>Connecting...</h1>\r\n\
<div class=\"spinner\"><h2>Yeee!</h2></div>\
<h1>WiFi connected!</h1>\r\n\
<h1>Please wait, connecting to MQTT broker...</h1>\r\n\
<meta http-equiv=\"refresh\" content=\"2\" >\
<h1> </h1>\r\n\
<p>RagingBits.</p>\r\n\
</body>\r\n\
</html>";

const char* end_webpage_error =  "<h1>Connected!</h1>\r\n\
<h1>Please reconnect to to your WiFi</h1>\r\n\
<h1> </h1>\r\n\
<p>RagingBits.</p>\r\n\
</body>\r\n\
</html>";

const char* ready_webpage =  "<!DOCTYPE html>\r\n\
<html lang=\"en\">\r\n\
<body>\r\n\
<h1>WiFi connected with</h1>\r\n\
<h1> IP: %u.%u.%u.%u </h1>\r\n\
<h1>MQTT Broker Connected.</h1>\r\n\
<h1> </h1>\r\n\
<h1>Ready 4Fun!</h1>\r\n\
<h1> </h1>\r\n\
<p>RagingBits.</p>\r\n\
</body>\r\n\
</html>";

/* WIFI Global Constants ----------------------------------------------------- */
const uint8_t WIFI_LOCAL_IP[4U] PROGMEM = {1U,2U,3U,4U};
const uint8_t WIFI_GATEWAY_IP[4U] PROGMEM = {1U,2U,3U,4U};
const uint8_t WIFI_SUBNET_MASK_IP[4U] PROGMEM = {255U,255U,255U,0U};
const uint8_t WIFI_HOTSPOT_PORT PROGMEM = 80U;

/* WIFI Global Variables ----------------------------------------------------- */
wifi_client_flags_t wifi_client_flags = {0U};
uint32_t wifi_client_connect_timeout = 0U;
uint32_t wifi_hotspot_timeout = 0U;


/* WIFI services -------------------------------------------------------------- */
//extern MqttClient mqtt_client;
IPAddress local_ip(WIFI_LOCAL_IP);
IPAddress gateway(WIFI_GATEWAY_IP);
IPAddress subnet(WIFI_SUBNET_MASK_IP);
WebServer wifi_server(WIFI_HOTSPOT_PORT);
Ticker wifi_client_thread;



/* WIFI External Functions ---------------------------------------------------- */
void WifiStart(void)
{
    if(0U == wifi_client_flags.initialised)
    {
        Serial.println(" ");
        Serial.println("Starting wifi.");  
        Serial.print("    Starting wifi thread... ");  
        wifi_client_thread.attach_ms(WIFI_THREAD_RATE, &WifiRunWork);
        wifi_client_flags.initialised = !0U;
        Serial.println("done.");  
    }

    
    if(0U == wifi_client_flags.reconnecting)
    {
        WiFi.disconnect();
        wifi_client_flags.reconnecting = !0U;
    }

    wifi_client_flags.webpage_to_show = 0;
}



void WifiRunWork(void)
{
    wifi_client_flags.work_run = !0U; 
}


void WifiWork(void)
{
    
    if(0U != wifi_client_flags.work_run)  
    {
        wifi_client_flags.work_run = 0U; 
      
        if(0U != wifi_client_flags.hotspot_running)
        {
            wifi_server.handleClient();
            
            if(MQTTIsConnected()) 
            {
                if(wifi_hotspot_timeout)
                {
                    wifi_hotspot_timeout--;
                }
                else
                {
                   wifi_client_flags.hotspot_running = 0U;
                   WifiSetHotspot(false);
                }
            }
            else
            {
                wifi_hotspot_timeout = WIFI_HOTSPOT_TIMEOUT;
            }
        }

        if(0U == wifi_client_flags.reconnecting)
        {
            if(WL_CONNECTED != WiFi.status())
            {
                wifi_client_flags.reconnecting = !0U;
            }
            else
            {
                if(MQTTIsConnected())  
                {
                    wifi_client_flags.webpage_to_show = 3;
                }
                else
                {
                    wifi_client_flags.webpage_to_show = 2;
                }
            }
        }
        else
        {
            WifiReConnect();
        }        
    }
}


bool WifiConnected(void)
{
    return (0U == wifi_client_flags.reconnecting);
}

IPAddress WifiGetIp(void)
{
    return WiFi.localIP();  
}

void WifiSetHotspot(bool enable)
{
    Serial.println(" ");
    if(enable)
    {
        
        Serial.print("Starting hotspot... ");  
        WiFi.softAP(userID_.c_str());
        WiFi.softAPConfig(local_ip, gateway, subnet);
        wifi_server.on(F("?"), handle_NotFound);
        wifi_server.onNotFound(handle_NotFound);
        wifi_server.begin();
        wifi_client_flags.hotspot_running = !0U;
    }
    else
    {
        Serial.print("Stopping hotspot...");
        wifi_server.stop();
        WiFi.softAPdisconnect(true);
        wifi_client_flags.hotspot_running = 0U;
    }

    Serial.println(" done.");  
            
}

/* WIFI Internal Functions ---------------------------------------------------- */
void WifiReConnect(void)
{
    char temp_data[PASS_MAX_LEN+SSID_MAX_LEN] = {0U};
    uint8_t temp_len = PASS_MAX_LEN;

    
    if(0U != wifi_client_flags.reconnecting)
    {
        if(0U == wifi_client_connect_timeout)
        {
            
            if(WL_CONNECTED == WiFi.status())
            {
                wifi_client_flags.reconnecting = 0U;
                wifi_hotspot_timeout = WIFI_HOTSPOT_TIMEOUT;
                Serial.println(" Connected.");  
                
            }
            else
            {
                wifi_client_connect_timeout = WIFI_CLIENT_RECONNECTION_TIMEOUT;

                 temp_len = PASS_MAX_LEN;
                 EepromRead(EEPROM_PARAPHRASE, (uint8_t*)temp_data, &temp_len);
                    
                if((0 != temp_data[0])&&(0xff != temp_data[0])) 
                {
                    temp_len = SSID_MAX_LEN;
                    EepromRead(EEPROM_SSID, (uint8_t*)&temp_data[PASS_MAX_LEN], &temp_len);

                    Serial.println(" ");
                    Serial.print("Connecting to wifi...");
                    
                    WiFi.begin(&temp_data[PASS_MAX_LEN], temp_data);
                }
                else
                {
                    temp_len = SSID_MAX_LEN;
                    EepromRead(EEPROM_SSID, (uint8_t*)temp_data, &temp_len);
                    WiFi.begin(temp_data, NULL); 
                }
            }
        }
        else
        {
            Serial.print(".");
            wifi_client_connect_timeout--;  
        }
    }
}


void handle_workOnConnect()
{
  handle_NotFound();
}

void handle_workNotFound()
{
  handle_NotFound();
}



void handle_NotFound() 
{
  char temp_data[1024];
  uint8_t temp_len = 0U;
  String result = wifi_server.arg("ssid");
  Serial.println("Wifi WEB request received: ");

  Serial.print("Connection state: ");Serial.println(wifi_client_flags.webpage_to_show);
  
  if(result != "" && 0U == wifi_client_flags.webpage_to_show)
  {
      wifi_server.arg("ssid").toCharArray(temp_data,SSID_MAX_LEN);
      EepromWrite(EEPROM_SSID, (uint8_t*)temp_data, SSID_MAX_LEN);
      
      wifi_server.arg("password").toCharArray(temp_data,PASS_MAX_LEN);
      EepromWrite(EEPROM_PARAPHRASE, (uint8_t*)temp_data, PASS_MAX_LEN);
      
      wifi_server.arg("broker_ip").toCharArray(temp_data,MQTT_BROKER_IP_MAX_LEN);
      EepromWrite(EEPROM_MQTT_BROKER_IP, (uint8_t*)temp_data, MQTT_BROKER_IP_MAX_LEN);
      
      wifi_server.arg("broker_port").toCharArray(temp_data, MQTT_BROKER_PORT_MAX_LEN);
      EepromWrite(EEPROM_MQTT_BROKER_PORT, (uint8_t*)temp_data, MQTT_BROKER_PORT_MAX_LEN);
      
      wifi_server.arg("rgb_format").toCharArray(temp_data,RGB_FORMAT_MAX_LEN);
      EepromWrite(EEPROM_RGB_FORMAT, (uint8_t*)temp_data, RGB_FORMAT_MAX_LEN);

      result = wifi_server.arg("uart3_speed");

      if(result != "")
      {
          result.toCharArray(temp_data,UART3_SPEED_MAX_LEN);      
          if(0 != atoi((char*)temp_data))
          {
              EepromWrite(EEPROM_UART3_SPEED, (uint8_t*)temp_data, UART3_SPEED_MAX_LEN);
              temp_data[0] = '1';
              temp_data[1] = 0U;
              EepromWrite(EEPROM_UART3_EN, (uint8_t*)temp_data, UART3_EN_MAX_LEN);  
          }        
      }
      else
      {
          temp_data[0] = '0';
          temp_data[1] = 0U;
          EepromWrite(EEPROM_UART3_EN, (uint8_t*)temp_data, UART3_EN_MAX_LEN);
      }

      temp_len = SSID_MAX_LEN;
      memset(temp_data,0U, SSID_MAX_LEN+1);
      EepromRead(EEPROM_SSID, (uint8_t*)temp_data, &temp_len);
      Serial.print("SSID - "); Serial.println(temp_data);
    
      temp_len = PASS_MAX_LEN;
      memset(temp_data,0U, PASS_MAX_LEN+1);
      EepromRead(EEPROM_PARAPHRASE, (uint8_t*)temp_data, &temp_len);
      Serial.print("PASS - "); Serial.println(temp_data);
    
      temp_len = MQTT_BROKER_IP_MAX_LEN;
      memset(temp_data,0U, MQTT_BROKER_IP_MAX_LEN+1);
      EepromRead(EEPROM_MQTT_BROKER_IP, (uint8_t*)temp_data, &temp_len);
      Serial.print("MQTT Broker ip - "); Serial.println(temp_data);
    
      temp_len = MQTT_BROKER_PORT_MAX_LEN;
      memset(temp_data,0U, MQTT_BROKER_PORT_MAX_LEN+1);
      EepromRead(EEPROM_MQTT_BROKER_PORT, (uint8_t*)temp_data, &temp_len);
      Serial.print("MQTT Broker port - "); Serial.println(temp_data);

      temp_len = RGB_FORMAT_MAX_LEN;
      memset(temp_data,0U, RGB_FORMAT_MAX_LEN+1);
      EepromRead(EEPROM_RGB_FORMAT, (uint8_t*)temp_data, &temp_len);
      Serial.print("RGB format - "); Serial.println(temp_data);

      temp_len = UART3_SPEED_MAX_LEN;
      memset(temp_data,0U, UART3_SPEED_MAX_LEN+1);
      EepromRead(EEPROM_UART3_SPEED, (uint8_t*)temp_data, &temp_len);
      Serial.print("Alternate UART Speed - "); Serial.println(temp_data);

      temp_len = UART3_EN_MAX_LEN;
      memset(temp_data,0U, UART3_EN_MAX_LEN+1);
      EepromRead(EEPROM_UART3_EN, (uint8_t*)temp_data, &temp_len);
      Serial.print("Alternate UART Enabled - "); Serial.println(temp_data);

      IOPinsForceUart3Check();
      
      wifi_client_flags.webpage_to_show = 1;
  }

  if(0U == wifi_client_flags.webpage_to_show)
  {
      Serial.println("Request SSID and PASS.");
      wifi_server.send(200, "text/html",request_webpage); 
      
  }
  else
  if(1U == wifi_client_flags.webpage_to_show)
  {
      Serial.print("Query on connection: ");
      Serial.println("Waiting wifi...");
      wifi_server.send(200, "text/html",wait_webpage_wifi); 
  }
  else      
  if(2U == wifi_client_flags.webpage_to_show)
  {
      Serial.print("Query on connection: ");
      Serial.println("Waiting mqtt...");
      wifi_server.send(200, "text/html",(char*)wait_webpage_mqtt); 
  }
  else  
  if(3U == wifi_client_flags.webpage_to_show)
  {
      Serial.print("Query on connection: ");
      Serial.println("IP webpage sent."); 
      IPAddress temp_ip = WifiGetIp();
      sprintf((char*)temp_data, ready_webpage, temp_ip[0], temp_ip[1], temp_ip[2], temp_ip[3]);
      wifi_server.send(200, "text/html",(char*)temp_data); 
  }
}
