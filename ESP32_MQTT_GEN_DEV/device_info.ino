

/* Main Device Unique Information ---------------------------------------- */

/* Device serial number. This is associated to every MQTT Topic and to Hotspot SSID name. 
   Several units may have the same Serial number in the same network, such they all receive the 
   same Topic data and respond with the same Topics too. 
   The length of the number MUST be respected. 10 digits EXACTLY. 
   */
   
//const String _SERIAL_NUMBER_  PROGMEM  = "0000000001";
//#pragma location=0x00000500
//const String *SERIAL_NUMBER_ = &_SERIAL_NUMBER_;//((String*)0x001BB000);

/* FIXED definitions. Do not change!!! ------------------------------------*/

String userID_   = "rb_wifi_mqtt_client_0000000000";

/* Main Device HW and SW Information ------------------------------------- */
#define HW_V_1  1
#define HW_V_2  2
#define HW_V_3  3

/* Current Hardware version defines pinouts. */
#define HARDWARE_VERSION  HW_V_2
