/*
 * WebSocketClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>
#include <ArduinoJson.h>

#include <WiFi.h>
//#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>
#include <U8g2lib.h>            // For text on the little on-chip OLED
#define FASTLED_INTERNAL        // Suppress build banner
#include <FastLED.h>

#define OLED_CLOCK  15          // Pins for the OLED display
#define OLED_DATA   4
#define OLED_RESET  16

U8G2_SSD1306_128X64_NONAME_F_HW_I2C g_OLED(U8G2_R2, OLED_RESET, OLED_CLOCK, OLED_DATA);
int g_lineHeight = 0;
int g_Brightness = 5;//15;         // 0-255 LED brightness scale
int g_PowerLimit = 50000;//900;        // 900mW Power Limit

//WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

#define FAN_SIZE      	236        		// Number of LEDs in each fan
#define NUM_FANS       	1        		// Number of Fans
#define LED_FAN_OFFSET 	0        		// How far from bottom first pixel is
#define NUM_LEDS      	236       		// FastLED definitions
#define LED_PIN        	5
#define RGB_TOGGLE_PIN 	0
#define BTM_LED_START	0				//0
#define BTM_LED_END		NUM_LEDS/3 		//78
#define MID_LED_START	BTM_LED_END 	//78
#define MID_LED_END		BTM_LED_END*2 	//157
#define TOP_LED_START	MID_LED_END		//157
#define TOP_LED_END		NUM_LEDS		//236

#include "ledgfx.h"
#include "comet.h"
#include "marquee.h"
#include "twinkle.h"
#include "fire.h"

enum animation{
	NONE,
	RED,
	BLUE,
	GREEN,
	ORANGE,
	GREEN_MAEQUEE
};

animation 	LT_AllianceBlue 	= 	NONE;
animation 	LT_AllianceRed 		= 	NONE;
animation 	LT_Field 			= 	NONE;
int 		LT_MatchState 		= 	0;
int 		LT_MatchTimeSec 	= 	0;

CRGB g_LEDs[NUM_LEDS] = {0};    // Frame buffer for FastLED
FireEffect fire(NUM_LEDS, 50, 100, 20, NUM_LEDS, true, false);

const char* ssid = "CheesyArenaAdmin";
const char* password = "1234Five";

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
	const uint8_t* src = (const uint8_t*) mem;
	USE_SERIAL.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
	for(uint32_t i = 0; i < len; i++) {
		if(i % cols == 0) {
			USE_SERIAL.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
		}
		USE_SERIAL.printf("%02X ", *src);
		src++;
	}
	USE_SERIAL.printf("\n");
}

//Return if a Station is Ready
bool stationReady(JsonObject data, String station){
	return data["AllianceStations"][station]["Bypass"] ||
			(data["AllianceStations"][station]["Ethernet"] &&
			 data["AllianceStations"][station]["TeamWifiStatuses"]["RadioLinked"]
			);
}

void parser(String s){
	
	DynamicJsonDocument doc(6145);
	deserializeJson(doc, s);
  	JsonObject json= doc.as<JsonObject>();

	const char* type = doc["type"]; // "arenaStatus, matchTime, ..."

	JsonObject data = doc["data"];	// Most Jason Files have a "data" section

	if(strcmp(type, "matchTime") == 0 ){
		LT_MatchTimeSec = data["MatchTimeSec"]; 
		LT_MatchState = data["MatchState"];
	}

	if(strcmp(type, "arenaStatus") == 0 ){

		LT_MatchState = data["MatchState"];
		
		bool b1_Ready = stationReady(data,"B1");
		bool b2_Ready = stationReady(data,"B2");
		bool b3_Ready = stationReady(data,"B3");
    	
		if(b1_Ready && b2_Ready && b3_Ready){
			LT_AllianceBlue = GREEN;
		}else{
			LT_AllianceBlue = BLUE;
		}

		bool r1_Ready = stationReady(data,"R1");
		bool r2_Ready = stationReady(data,"R2");
		bool r3_Ready = stationReady(data,"R3");
    	
		if(r1_Ready && r2_Ready && r3_Ready){
			LT_AllianceRed = GREEN;
		}else{
			LT_AllianceRed = RED;
		}
		
		bool data_CanStartMatch = data["CanStartMatch"];
		if(data_CanStartMatch){
			LT_Field = GREEN_MAEQUEE;
		}else{
			LT_Field = ORANGE;
		}
	}
}

//A string to concat the socketData together
String socketData;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
	//USE_SERIAL.printf("type: %x\n", type);
			
	switch(type) {
		case WStype_DISCONNECTED:
			USE_SERIAL.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED:
			USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);
			break;
		case WStype_TEXT:
			USE_SERIAL.printf("[WSc] get text: %s\n", payload);
			socketData = (char * )payload;
			parser(socketData);
			break;
		case WStype_BIN:
			USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
			hexdump(payload, length);
			break;
		case WStype_ERROR:	
			USE_SERIAL.println("[************WSc ERROR***********]");		
		case WStype_FRAGMENT_TEXT_START:
			socketData = (char * )payload;
			//USE_SERIAL.printf("[WSc] Fragment Text Start: %s\n", payload);
			//USE_SERIAL.println(socketData);
			//USE_SERIAL.println("WStype_FRAGMENT_TEXT_START");
			break;
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
			socketData += (char * )payload;
			break;
		case WStype_FRAGMENT_FIN:
			socketData += (char * )payload; 
			parser(socketData);
			//USE_SERIAL.println(socketData);
			break;
		case WStype_PONG:
			//USE_SERIAL.printf("[WSc] WStype_PONG: Ping reply\n");
			break;
	}

}


IPAddress local_ip(10,0,100,10);
IPAddress gateway(10,0,100,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8,8,8,8);
IPAddress secondaryDNS(8,8,4,4);
void intiWifi(){
	WiFi.mode(WIFI_STA);
	WiFi.config(local_ip,gateway,subnet,primaryDNS,secondaryDNS);
	WiFi.begin(ssid,password);
	USE_SERIAL.print("Connecting to WiFi .. ");
	while(WiFi.status() != WL_CONNECTED){
		USE_SERIAL.print('.');
		delay(1000);
	}
	//WiFi.reconnect();
	USE_SERIAL.println(WiFi.localIP());
	delay(3000);
}


void setup() {
	
	pinMode(LED_BUILTIN, OUTPUT);
  	pinMode(LED_PIN, OUTPUT);
	// USE_SERIAL.begin(921600);
	USE_SERIAL.begin(115200);

	//Serial.setDebugOutput(true);
	USE_SERIAL.setDebugOutput(true);

	USE_SERIAL.println("set up oled");
	g_OLED.begin();
  	g_OLED.clear();
  	g_OLED.setFont(u8g2_font_profont15_tf);
  	g_lineHeight = g_OLED.getFontAscent() - g_OLED.getFontDescent();        // Descent is a negative number so we add it to the total

	FastLED.addLeds<WS2812B, LED_PIN, GRB>(g_LEDs, NUM_LEDS);               // Add our LED strip to the FastLED library
	FastLED.setBrightness(g_Brightness);
  	set_max_power_indicator_LED(LED_BUILTIN);                               // Light the builtin LED if we power throttle
  	FastLED.setMaxPowerInMilliWatts(g_PowerLimit);                          // Set the power limit, above which brightness will be throttled

	intiWifi();

  	USE_SERIAL.println("Connect WebSocket");

  	webSocket.setExtraHeaders("Origin: http://10.0.100.5:8080");
  
	// server address, port and URL
	//webSocket.begin("10.0.100.5", 8080, "ws://10.0.100.5:8080/displays/field_monitor/websocket?displayId=101&ds=false&fta=false&reversed=false");
  	//webSocket.begin("10.0.100.5", 8080, "ws://10.0.100.5:8080/displays/alliance_station/websocket?displayId=100&station=R1.");
	webSocket.begin("10.0.100.5", 8080, "ws://10.0.100.5:8080/match_play/websocket");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	//webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);
}


void MatchStatePre(){
	switch (LT_AllianceBlue){
		case NONE:
			for (int i = BTM_LED_START; i < BTM_LED_END; i++){
				DrawPixels(i, 1, CRGB(0,0,0));
			}
			break;
		case RED:
			for (int i = BTM_LED_START; i < BTM_LED_END; i++){
				DrawPixels(i, 1, CRGB(255,0,0));
			}
			break;
		case BLUE:
			for (int i = BTM_LED_START; i < BTM_LED_END; i++){
				DrawPixels(i, 1, CRGB(0,0,255));
			}
			break;
		case GREEN:
			for (int i = BTM_LED_START; i < BTM_LED_END; i++){
				DrawPixels(i, 1, CRGB(0,255,0));
			}
			break;
		default:
			for (int i = BTM_LED_START; i < BTM_LED_END; i++){
				DrawPixels(i, 1, CRGB(100,100,100));
			}
    		break;
    }
	switch (LT_AllianceRed){
		case NONE:
			for (int i = MID_LED_START; i < MID_LED_END; i++){
				DrawPixels(i, 1, CRGB(0,0,0));
			}
			break;
		case RED:
			for (int i = MID_LED_START; i < MID_LED_END; i++){
				DrawPixels(i, 1, CRGB(255,0,0));
			}
			break;
		case BLUE:
			for (int i = MID_LED_START; i < MID_LED_END; i++){
				DrawPixels(i, 1, CRGB(0,0,255));
			}
			break;
		case GREEN:
			for (int i = MID_LED_START; i < MID_LED_END; i++){
				DrawPixels(i, 1, CRGB(0,255,0));
			}
			break;
		default:
			for (int i = MID_LED_START; i < MID_LED_END; i++){
				DrawPixels(i, 1, CRGB(100,100,100));
			}
    		break;
    } 
	switch (LT_Field){
		case NONE:
			for (int i = TOP_LED_START; i < TOP_LED_END; i++){
				DrawPixels(i, 1, CRGB(0,0,0));
			}
			break;
		case RED:
			for (int i = TOP_LED_START; i < TOP_LED_END; i++){
				DrawPixels(i, 1, CRGB(0,0,255));
			}
			break;
		case BLUE:
			for (int i = TOP_LED_START; i < TOP_LED_END; i++){
				DrawPixels(i, 1, CRGB(0,0,255));
			}
			break;
		case GREEN:
			for (int i = TOP_LED_START; i < TOP_LED_END; i++){
				DrawPixels(i, 1, CRGB(0,255,0));
			}
			break;
		case ORANGE:
			for (int i = TOP_LED_START; i < TOP_LED_END; i++){
				DrawPixels(i, 1, CRGB(255,70,0));
			}
			break;
		case GREEN_MAEQUEE:
			DrawMarquee(CRGB(0,255,0));
			break;
		default:
			for (int i = TOP_LED_START; i < TOP_LED_END; i++){
				DrawPixels(i, 1, CRGB(100,100,100));
			}
    		break;
    }
}

void loop() {
	webSocket.loop();
	FastLED.clear();
 	
	int currentLastLED = 0;
	int endGameLED = 0;
	int autoLED = 0;
	CRGB color = CRGB(0,0,0);
	switch(LT_MatchState){
		case 0: //Pre Match
			MatchStatePre();
			break;
		case 1 ... 5: //In Match
			currentLastLED = (((float)LT_MatchTimeSec/(float)153)*(float)NUM_LEDS);
			endGameLED = 187;//(((float)123/(float)153)*(float)NUM_LEDS);
			autoLED = 22;//(((float)15/(float)153)*(float)NUM_LEDS);
			for (int i = BTM_LED_START; i < currentLastLED; i++){
				if(i>endGameLED){
					color = CRGB(255,0,0);
				}else if(i>autoLED){
					color = CRGB(255,255,0);
				}else{
					color = CRGB(0,255,0);
				}
				DrawPixels(i, 1, color);
			} 
			break;
		case 6: //Post Match
			for (int i = BTM_LED_START; i < TOP_LED_END; i++){
				DrawPixels(i, 1, CRGB(148,0,211));
			}
		default:
			//Do Nothing
			break;
	}
			
	FastLED.show(g_Brightness); //  Show and delay
	
	EVERY_N_MILLISECONDS(250){
      g_OLED.clearBuffer();
      g_OLED.setCursor(0, g_lineHeight);
      g_OLED.printf("FPS  : %u", FastLED.getFPS());
      g_OLED.setCursor(0, g_lineHeight * 2);
      g_OLED.printf("Power: %u mW", calculate_unscaled_power_mW(g_LEDs, 4));
      g_OLED.setCursor(0, g_lineHeight * 3);
      g_OLED.printf("Brite: %d", calculate_max_brightness_for_power_mW(g_Brightness, g_PowerLimit));
      g_OLED.setCursor(0, g_lineHeight * 4);
      g_OLED.printf("Local ip: %s", WiFi.localIP());
      g_OLED.sendBuffer();
	  USE_SERIAL.println(WiFi.status()); 
    }
}