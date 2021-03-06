#include <EEPROM.h>
#include <ESP8266.h>
#include <aJSON.h>

#define MQTTSERVER   "mCotton.microduino.cn"
#define PROJECTID   "58f881ae42c66e00013cf3cc"
#define PROJECTTOKEN "j9eVXErQdnr4"
#define deviceName "rayWS"

#define MQTT_PORT   (1883)
#define REG_PORT   (1881)
#define DeviceID_Token_Range 200
#define EEPROMLengh 1024
#define smartConfigerButtonPin 5

bool isSmartConfiger = false;
bool getDevID_TokenEnable = false;


#define INTERVAL_sensor 2000
unsigned long sensorlastTime = millis();

String mCottenData;
String jsonData;
String subscribTopic;
String publishTopic;
String MACAddr;
String deviceID, secureToken;
const uint8_t lenNum = 5;

ESP8266 wifi(Serial1, 115200);


//写EEPROM
void writeToEEPROM(uint16_t start, uint16_t end) {
	//clear EEPROM
	for (uint16_t i = start; i < end; i++) {
		EEPROM.write(i, 0);
	}
//	EEPROM.commit();
//		EEPROM.end();
	String storInfo;

	storInfo = deviceID + "," + secureToken + ",";

	//write
	uint16_t storInfoLen = storInfo.length();
	storInfo = String(storInfoLen) + "," + storInfo;
	for (uint16_t i = start; i < (start + storInfo.length()); i++) {
		EEPROM.write(i, storInfo[i]);
	}
	Serial.print("---write from ");
	Serial.print(start);
	Serial.println(" once---");
//	EEPROM.commit();
//		EEPROM.end();
}

//显示EEPROM
void showEEPROM() {
	Serial.println();
	Serial.print("Total memory is:");
	Serial.println(EEPROMLengh);
	Serial.println("-----EEPROM image here:-----");
	for (uint16_t i = 0; i < EEPROMLengh; i++) {
		char ascii = EEPROM.read(i);
		Serial.print(ascii);
	}
	Serial.println();
//		EEPROM.commit();
//		EEPROM.end();
}

//读EEPROM
void readEEPROM(uint8_t start, uint8_t end) {
	//get lenNum
	String lenNumStr;
	for (uint8_t i = start; i < (start + lenNum); i++) {
		char ascii = EEPROM.read(i);
		if (i == 0) {
			lenNumStr = ascii;
		} else {
			if (EEPROM.read(i) == ',') {
				break;
			} else {
				lenNumStr += ascii;
			}
		}
	}

	uint8_t startIndex = start + lenNumStr.length() + 1;
	uint16_t lenNumber = lenNumStr.toInt();

	String praseStr;
	for (uint16_t i = startIndex; i < lenNumber + startIndex; i++) {
		char printChar = EEPROM.read(i);
		if (praseStr.length() > 0) {
			praseStr += printChar;
		} else {
			praseStr = printChar;
		}
	}
//		EEPROM.commit();
//		EEPROM.end();
	String printStr;
	int strLength = praseStr.length();
	int startSplit = 0;
	uint8_t num = 0;
	for (uint16_t i = 0; i < strLength; i++) {

		Serial.println(i);
		if (praseStr.substring(i, i + 1) == ",") {
			printStr = praseStr.substring(startSplit, i);
			startSplit = i + 1;
			switch (num) {
			case 0:
				Serial.print("diviceID:");
				deviceID = printStr;
				Serial.println(printStr);
				break;
			case 1:
				Serial.print("secureToken:");
				secureToken = printStr;
				Serial.println(printStr);
				break;
			}
			num++;
		}
	}
//		EEPROM.commit();
//		EEPROM.end();
}

void doRegMqtt() {

//	Serial.println("station MAC addrress:");
//	MACAddr=wifi.getStationMac();
//	MACAddr.replace(":", "");
//	MACAddr.toLowerCase();
//	Serial.println(MACAddr);

	subscribTopic = "s\/";
	subscribTopic += PROJECTID;
	subscribTopic += "\/";
	subscribTopic += MACAddr;

	publishTopic = "q\/";
	publishTopic += PROJECTID;
	publishTopic += "\/";
	publishTopic += MACAddr;

	//setRegister
	if (wifi.mqttSetServer(MQTTSERVER, REG_PORT)) {
		Serial.print("mqtt set reg server ok\r\n");
	} else {
		Serial.print("mqtt set reg server err\r\n");
	}

	if (wifi.mqttConnect(MACAddr, PROJECTID, PROJECTTOKEN)) {
		wifi.setMqttConnected(true);
		Serial.print("mqtt reg connect ok\r\n");
	} else {
		wifi.setMqttConnected(false);
		Serial.print("mqtt reg connect err\r\n");
	}

	if (wifi.mqttSetSubscrib(subscribTopic)) {
		Serial.print("reg set subscrib topic ok\r\n");
	} else {
		Serial.print("reg set subscrib topic err\r\n");
	}

	jsonData = "{\"";
	jsonData += "dM\":\"";
	jsonData += MACAddr;
	jsonData += "\",\"pId\":\"";
	jsonData += PROJECTID;
	jsonData += "\",\"dN\":\"";
	jsonData += deviceName;
	jsonData += "\"}";

	if (wifi.mqttSetTopic(publishTopic)) {
		Serial.print("mqtt reg set publish topic ok\r\n");
	} else {
		Serial.print("mqtt reg set publish topic err\r\n");
	}
	if (wifi.mqttSetMessage(jsonData)) {
		Serial.print("mqtt reg set message ok\r\n");
	} else {
		Serial.print("mqtt reg set message err\r\n");
	}

	if (wifi.mqttPub()) {
		Serial.print("mqtt reg pub ok\r\n");
	} else {
		Serial.print("mqtt reg pub err\r\n");
	}


	Serial.println("Waitting for DeviceID, DeviceToken");



	while (true) {
		if (sensorlastTime > millis())
			sensorlastTime = millis();
		if (millis() - sensorlastTime > INTERVAL_sensor) {
			Serial.println("...");
			sensorlastTime = millis();
		}
		mCottenData = wifi.getMqttJson();
		if (mCottenData != "") {
			mCottenData.trim();
			if (mCottenData.startsWith("{") && mCottenData.endsWith("}")) {
				uint8_t length = mCottenData.length();
				char buf[length + 1];
				mCottenData.toCharArray(buf, length + 1);
				aJsonObject* root = aJson.parse(buf);
				aJsonObject* DID = aJson.getObjectItem(root, "deviceId");
				aJsonObject* DToken = aJson.getObjectItem(root, "secureToken");
				deviceID = DID->valuestring;
				secureToken = DToken->valuestring;
				break;
			}
		}
	}


}


//设置传输数据Mqtt
void setTransportMqtt() {
	readEEPROM(0, DeviceID_Token_Range);

	if (wifi.mqttSetServer(MQTTSERVER, MQTT_PORT)) {
		Serial.print("mqtt set server ok\r\n");
	} else {
		Serial.print("mqtt set server err\r\n");
	}

	//subscribe:ca

	//publish:dp->数据
	//da->控制
	//		:de->事件

	subscribTopic = "ca\/";
	subscribTopic += deviceID;

	publishTopic = "dp\/";
	publishTopic += deviceID;


	if (wifi.mqttConnect(MACAddr, deviceID, secureToken)) {
		wifi.setMqttConnected(true);
		Serial.print("mqtt connect ok\r\n");
	} else {
		wifi.setMqttConnected(false);
		Serial.print("mqtt connect err\r\n");
	}
	if (wifi.mqttSetDiveceIDToken(MACAddr, secureToken)) {
		Serial.print("mqtt set device ID Token ok\r\n");
	} else {
		Serial.print("mqtt set device ID Token err\r\n");
	}
	if (wifi.mqttSetSubscrib(subscribTopic)) {
		Serial.print("mqtt set subscrib ca topic ok\r\n");
	} else {
		Serial.print("mqtt set subscrib ca topic err\r\n");
	}

	subscribTopic[1]='p';
	if (wifi.mqttSetSubscrib(subscribTopic)) {
		Serial.print("mqtt set subscrib cp topic ok\r\n");
	} else {
		Serial.print("mqtt set subscrib cp topic err\r\n");
	}
	subscribTopic[1]='a';

}

void setup(void) {
	Serial.begin(115200);


	pinMode(smartConfigerButtonPin, INPUT_PULLUP);
	bool smartState = digitalRead(smartConfigerButtonPin);
	isSmartConfiger = getDevID_TokenEnable = !smartState;


//	EEPROM.begin(EEPROMLengh);
	EEPROM.begin();
	Serial.print("FW Microduino Version:");
	Serial.println(wifi.getMVersion());

	if (wifi.setOprToStation()) {
		Serial.print("to station ok\r\n");
	} else {
		Serial.print("to station err\r\n");
	}

	Serial.println("station MAC addrress:");
	MACAddr=wifi.getStationMac();
	MACAddr.replace(":", "");
	MACAddr.toLowerCase();
	Serial.println(MACAddr);

	if (isSmartConfiger) {
		wifi.smartConfiger(true);
		Serial.println("Smart Configer ...");
		while (true) {

			if (sensorlastTime > millis())
				sensorlastTime = millis();
			if (millis() - sensorlastTime > INTERVAL_sensor) {

				Serial.println("...");
				sensorlastTime = millis();
			}

			mCottenData = wifi.getMqttJson();
			if (mCottenData != "") {
				mCottenData.trim();
				Serial.println(mCottenData);
				if (mCottenData == "smartconfig connected wifi") {
					break;
				}
			}
		}
	} else {
		wifi.setAutoConnect(true);
		Serial.println("restart");
		wifi.restart();
	}

	Serial.println("wifi info");
	Serial.println(wifi.queryWiFiInfo());

	if (getDevID_TokenEnable) {
		//执行注册
		doRegMqtt();

		writeToEEPROM(0, DeviceID_Token_Range);
		if(wifi.mqttDisconnect()) {//断开连接
			Serial.print("disconnect ok\r\n");
		}else {
			Serial.print("disconnect err\r\n");
		}

	} else {//否则，直接设置传输数据Mqtt
		showEEPROM();
		delay(1000);//??????
	}
	//设置传输数据Mqtt
	setTransportMqtt();
}

void loop(void) {

	mCottenData = wifi.getMqttJson();

	if (mCottenData != "") {
		mCottenData.trim();
		Serial.println(mCottenData);
		if (mCottenData.equals("WIFI DISCONNECT")) {
			wifi.setWiFiconnected(false);
			wifi.setMqttConnected(false);
		} else if (mCottenData.equals("MQTT: Connected")) {
			wifi.setWiFiconnected(true);
		} else if (mCottenData.equals("MQTT: Disconnected")) {
			wifi.setMqttConnected(false);
		} else if (mCottenData.equals("MQTT: Connected")) {
			wifi.setMqttConnected(true);
		}
		if (!wifi.isWiFiconnected()) {
			return;
		}
		if (wifi.isMqttConnected()) {
			if (mCottenData.startsWith("{") && mCottenData.endsWith("}")) {

				uint8_t length = mCottenData.length();
				char buf[length + 1];
				mCottenData.toCharArray(buf, length + 1);

				aJsonObject* root = aJson.parse(buf);
				aJsonObject* state = aJson.getObjectItem(root, "undefined");

				if (strcmp(state->valuestring, "true") == 0) {
					Serial.println("ON");
				}
				if (strcmp(state->valuestring, "false") == 0) {
					Serial.println("OFF");
				}
			}
		}
	}

	if (wifi.isMqttConnected()) {
		if (sensorlastTime > millis())
			sensorlastTime = millis();
		if (millis() - sensorlastTime > INTERVAL_sensor) {

			uint16_t value = analogRead(A0);

			jsonData = "{\"Humidity\":";
			jsonData += value / 2;
			jsonData += ",\"Temperature\":";
			jsonData += value / 4;
			jsonData += ",\"Lightness\":";
			jsonData += value;
			jsonData += "}";

			if (wifi.mqttPublishM(jsonData)) {
				Serial.print("mqtt publishM ok\r\n");
			} else {
				Serial.print("mqtt publishM err\r\n");
			}

			sensorlastTime = millis();

		}
	}

}

