/*
 * WebSocketClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>
#include <ArduinoJson.h>

#include <WiFi.h>
#include <WiFiMulti.h>
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
int g_Brightness = 10;//15;         // 0-255 LED brightness scale
int g_PowerLimit = 50000;//900;        // 900mW Power Limit

WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

#define FAN_SIZE      236        // Number of LEDs in each fan
#define NUM_FANS       1        // Number of Fans
#define LED_FAN_OFFSET 0        // How far from bottom first pixel is
#define NUM_LEDS      236       // FastLED definitions
#define LED_PIN        5
#define RGB_TOGGLE_PIN 0
#define BTM_LED_START	0
#define BTM_LED_END		NUM_LEDS/3
#define MID_LED_START	BTM_LED_END+1
#define MID_LED_END		BTM_LED_END*2
#define TOP_LED_START	MID_LED_END+1
#define TOP_LED_END		NUM_LEDS

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
	GREEN_MAEQUEE
};

animation alianceBlue = NONE;

CRGB g_LEDs[NUM_LEDS] = {0};    // Frame buffer for FastLED
FireEffect fire(NUM_LEDS, 50, 100, 20, NUM_LEDS, true, false);    
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

	const char* type = doc["type"]; // "arenaStatus"

	JsonObject data = doc["data"];

	if(strcmp(type, "arenaStatus") == 0 ){
		FastLED.clear();
		int data_MatchId = data["MatchId"]; // 1
		int data_MatchState = data["MatchState"]; // 0
		
		bool b1_Ready = stationReady(data,"B1");
		bool b2_Ready = stationReady(data,"B2");
		bool b3_Ready = stationReady(data,"B3");
    	CRGB m_color;// = CRGB(255,0,0);
		if(b1_Ready && b2_Ready && b3_Ready){
			m_color = CRGB(0,255,0);
		}else{
			m_color = CRGB(0,0,255);
		}
		for (int i = BTM_LED_START; i < BTM_LED_END; i++){
			DrawPixels(i, 1, m_color);
		}

		bool r1_Ready = stationReady(data,"R1");
		bool r2_Ready = stationReady(data,"R2");
		bool r3_Ready = stationReady(data,"R3");
    	
		if(r1_Ready && r2_Ready && r3_Ready){
			m_color = CRGB(0,255,0);
		}else{
			m_color = CRGB(255,0,0);
		}
		for (int i = MID_LED_START; i < MID_LED_END; i++){
			DrawPixels(i, 1, m_color);
		}
		
		bool data_CanStartMatch = data["CanStartMatch"]; // false
		if(data_CanStartMatch){
			//m_color = CRGB(0,255,0);
			FastLED.clear();
			//fire.DrawFire();
			DrawMarquee(CRGB(0,255,0));
		}else{
			m_color = CRGB(255,70,0);
		for (int i = TOP_LED_START; i < TOP_LED_END; i++){
			DrawPixels(i, 1, m_color);
		}
		}
		
		
		
	
	
	}

/* if(data_CanStartMatch||bp>7){
	USE_SERIAL.println("Start Match");
    m_color= CRGB(0,255,0);
}else{
    m_color= CRGB(0,0,255);
	USE_SERIAL.println("NOT Start Match");
}
//	USE_SERIAL.printf("[Paser] heu: %s\n", hue);
	for (int i = 0; i < NUM_LEDS; i++){
       DrawPixels(i, 1, m_color);
	} */
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

			// send message to server when Connected
			// webSocket.sendTXT("Connected");
			break;
		case WStype_TEXT:
			USE_SERIAL.printf("[WSc] get text: %s\n", payload);

			// send message to server
			// webSocket.sendTXT("message here");
			break;
		case WStype_BIN:
			USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
			hexdump(payload, length);

			// send data to server
			// webSocket.sendBIN(payload, length);
			break;
		case WStype_ERROR:			
		case WStype_FRAGMENT_TEXT_START:
			socketData = (char * )payload;
			//USE_SERIAL.printf("[WSc] Fragment Text Start: %s\n", payload);
			//USE_SERIAL.println(socketData);
			//USE_SERIAL.println("WStype_FRAGMENT_TEXT_START");
			break;
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
			socketData += (char * )payload;
			//USE_SERIAL.println("WStype_FRAGMENT");
			break;
		case WStype_FRAGMENT_FIN:
			
			socketData += (char * )payload; 
			parser(socketData);
			//USE_SERIAL.println(socketData);
			//USE_SERIAL.println("WStype_FRAGMENT_TEXT_FIN");
			//USE_SERIAL.printf("[WSc] get text: %s\n", payload);
			break;
		case WStype_PONG:
			//USE_SERIAL.printf("[WSc] WStype_PONG: Ping reply\n");
			break;

			
	}

}

void setup() {
	
	pinMode(LED_BUILTIN, OUTPUT);
  	pinMode(LED_PIN, OUTPUT);
	// USE_SERIAL.begin(921600);
	USE_SERIAL.begin(115200);

	//Serial.setDebugOutput(true);
	USE_SERIAL.setDebugOutput(true);

	USE_SERIAL.println();
	USE_SERIAL.println();
	USE_SERIAL.println();

	USE_SERIAL.println("set up oled");
	g_OLED.begin();
  g_OLED.clear();
  g_OLED.setFont(u8g2_font_profont15_tf);
  g_lineHeight = g_OLED.getFontAscent() - g_OLED.getFontDescent();        // Descent is a negative number so we add it to the total

FastLED.addLeds<WS2812B, LED_PIN, GRB>(g_LEDs, NUM_LEDS);               // Add our LED strip to the FastLED library
  FastLED.setBrightness(g_Brightness);
  set_max_power_indicator_LED(LED_BUILTIN);                               // Light the builtin LED if we power throttle
  FastLED.setMaxPowerInMilliWatts(g_PowerLimit);                          // Set the power limit, above which brightness will be throttled
  

	for(uint8_t t = 4; t > 0; t--) {
		USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
		USE_SERIAL.flush();
		delay(100);
	}

	WiFiMulti.addAP("AppleHome", "ApplefamilyWiFi");

	//WiFi.disconnect();
	while(WiFiMulti.run() != WL_CONNECTED) {
    USE_SERIAL.println("Connecting to WiFi");
	g_OLED.setCursor(0, g_lineHeight);
	g_OLED.printf("Connecting to WiFi");
	g_OLED.sendBuffer();
		delay(100);
	}
  USE_SERIAL.println("WiFi IP:");
  USE_SERIAL.println(WiFi.localIP());

  USE_SERIAL.println("Connect WebSocket");

  webSocket.setExtraHeaders("Origin: http://192.168.1.239:8080");
  
	// server address, port and URL
	//webSocket.begin("192.168.1.239", 8080, "ws://192.168.1.239:8080/displays/field_monitor/websocket?displayId=101&ds=false&fta=false&reversed=false");
  //webSocket.begin("192.168.1.239", 8080, "ws://192.168.1.239:8080/displays/alliance_station/websocket?displayId=100&station=R1.");
	webSocket.begin("192.168.1.239", 8080, "ws://192.168.1.239:8080/match_play/websocket");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	//webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);

}

	

void loop() {
	DrawMarquee(CRGB(0,255,0));
	webSocket.loop();
	FastLED.show(g_Brightness); //  Show and delay
	EVERY_N_MILLISECONDS(250)
    {
      g_OLED.clearBuffer();
      g_OLED.setCursor(0, g_lineHeight);
      //g_OLED.printf("FPS  : ");
	  bool success = webSocket.sendPing();

	  if(success){
		g_OLED.printf("true");
	  }else{
		g_OLED.printf("false");

	  }
	  
      //g_OLED.printf("FPS  : %u", FastLED.getFPS());
      //g_OLED.setCursor(0, g_lineHeight * 2);
      //g_OLED.printf("Power: %u mW", calculate_unscaled_power_mW(g_LEDs, 4));
      //g_OLED.setCursor(0, g_lineHeight * 3);
      //g_OLED.printf("Brite: %d", calculate_max_brightness_for_power_mW(g_Brightness, g_PowerLimit));
      //g_OLED.setCursor(0, g_lineHeight * 4);
      //g_OLED.printf("show_Pointer: %d", show_Pointer);
      g_OLED.sendBuffer();
    }
}