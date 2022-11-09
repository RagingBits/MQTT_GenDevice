Using the ESP32_Wroom_Tools\Programmer
¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨
1) Connect the PC serial port to the UART pins in the MQTT device.

2) Enter programming mode:
	Press both reset and IO-0 buttons at the same time. Release Reset button first, wait 1 second and release IO-0 button lastely.

3) Edit RunProgrammerConfig.txt with:
	The path to MQTT_RB_Magic_Lights\bin\ESP32_MQTT_LIGHTS_V2.2.bin.
	The correct serial port

4) Run RunProgrammer.bat
