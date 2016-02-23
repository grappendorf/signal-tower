/**
 * This file is part of the signal-tower firmware.
 *
 * (c) 2016 Dirk Grappendorf (www.grappendorf.net)
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROMex.h>
#include <avr/wdt.h>

const uint8_t PIN_GREEN = 13;
const uint8_t PIN_RED = 12;
const uint8_t PIN_YELLOW = 11;
const uint8_t PIN_AMBIENT = A0;

bool green;
bool red;
bool yellow;
bool mute;
int ambient;

uint16_t magic;
uint8_t threshold;
uint8_t hysteresis;
const uint16_t MAGIC_NUMBER = 0xCAFE;
const uint8_t DEFAULT_THRESHOLD = 100;
const uint8_t DEFAULT_HYSTERESIS = 5;
const uint16_t EEPROM_SIZE = EEPROMSizeATmega328;
const uint16_t EEPROM_MAGIC_ADDR = 0;
const uint16_t EEPROM_THRESHOLD_ADDR = (EEPROM_MAGIC_ADDR + sizeof(magic));
const uint16_t EEPROM_HYSTERESIS_ADDR = (EEPROM_THRESHOLD_ADDR + sizeof(hysteresis));

enum Method {
  GET, PUT
};
int contentLength;
Method method;
String path;

const int BUFFER_LENGTH = 100;
char buffer[BUFFER_LENGTH + 1];

void readEeprom();
void updateLeds();
void processSensors();
void getSensors();
void getLeds();
void putLeds();
void getSettings();
void putSettings();
void selfTest();
void parseHttp();
String parsePath(String line);
String readContent();
void sendResponse(int status, String content);
void sendResponse(int status, JsonObject &content);
void sendResponse(int status);

void setup() {
  EEPROM.setMemPool(0, EEPROM_SIZE);
  readEeprom();

  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);

  green = false;
  red = false;
  yellow = false;
  mute = false;
  contentLength = 0;

  selfTest();
  updateLeds();
  Serial.begin(57600);
  wdt_enable(WDTO_4S);
}

void loop() {
  wdt_reset();

  if (Serial.available()) {
    parseHttp();
  }

  processSensors();
}

void readEeprom() {
  magic = EEPROM.readInt(EEPROM_MAGIC_ADDR);
  if (magic != MAGIC_NUMBER) {
    EEPROM.writeInt(EEPROM_MAGIC_ADDR, MAGIC_NUMBER);
    hysteresis = DEFAULT_HYSTERESIS;
    EEPROM.writeBlock(EEPROM_THRESHOLD_ADDR, &threshold, sizeof(threshold));
    threshold = DEFAULT_THRESHOLD;
    EEPROM.writeBlock(EEPROM_HYSTERESIS_ADDR, &hysteresis, sizeof(hysteresis));
  } else {
    EEPROM.readBlock(EEPROM_THRESHOLD_ADDR, &threshold, sizeof(threshold));
    EEPROM.readBlock(EEPROM_HYSTERESIS_ADDR, &hysteresis, sizeof(hysteresis));
  }
}

void updateLeds() {
  digitalWrite(PIN_GREEN, (green && !mute) ? HIGH : LOW);
  digitalWrite(PIN_RED, (red && !mute) ? HIGH : LOW);
  digitalWrite(PIN_YELLOW, (yellow && !mute) ? HIGH : LOW);
}

void processSensors() {
  ambient = (int) map(analogRead(PIN_AMBIENT), 0, 1023, 0, 255);
  if (mute && ambient > threshold + hysteresis) {
    mute = false;
    updateLeds();
  } else if (!mute && ambient < threshold - hysteresis) {
    mute = true;
    updateLeds();
  }
}

void getSensors() {
  StaticJsonBuffer<BUFFER_LENGTH + 1> jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  json["ambient"] = ambient;
  sendResponse(200, json);
}

void getLeds() {
  StaticJsonBuffer<BUFFER_LENGTH + 1> jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  json["green"] = green;
  json["yellow"] = yellow;
  json["red"] = red;
  json["muted"] = mute;
  sendResponse(200, json);
}

void putLeds() {
  String content = readContent();
  StaticJsonBuffer<BUFFER_LENGTH + 1> jsonBuffer;
  JsonObject &json = jsonBuffer.parseObject(content);
  if (json.success()) {
    if (json.containsKey("green")) {
      green = json["green"];
    }
    if (json.containsKey("red")) {
      red = json["red"];
    }
    if (json.containsKey("yellow")) {
      yellow = json["yellow"];
    }
    updateLeds();
    sendResponse(204);
  } else {
    sendResponse(400);
  }
}

void getSettings() {
  StaticJsonBuffer<BUFFER_LENGTH + 1> jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  json["threshold"] = threshold;
  json["hysteresis"] = hysteresis;
  sendResponse(200, json);
}

void putSettings() {
  String content = readContent();
  StaticJsonBuffer<BUFFER_LENGTH + 1> jsonBuffer;
  JsonObject &json = jsonBuffer.parseObject(content);
  if (json.success()) {
    if (json.containsKey("threshold")) {
      threshold = max(0, min(255, json["threshold"]));
      EEPROM.writeBlock(EEPROM_THRESHOLD_ADDR, &threshold, sizeof(threshold));
    }
    if (json.containsKey("hysteresis")) {
      hysteresis = max(0, min(255, json["hysteresis"]));
      EEPROM.writeBlock(EEPROM_HYSTERESIS_ADDR, &hysteresis, sizeof(hysteresis));
    }
    sendResponse(204);
  } else {
    sendResponse(400);
  }
}

void selfTest() {
  for (int i = 0; i < 4; ++i) {
    digitalWrite(PIN_GREEN, HIGH);
    delay(200);
    digitalWrite(PIN_GREEN, LOW);
    digitalWrite(PIN_YELLOW, HIGH);
    delay(200);
    digitalWrite(PIN_YELLOW, LOW);
    digitalWrite(PIN_RED, HIGH);
    delay(200);
    digitalWrite(PIN_RED, LOW);
  }
}

void parseHttp() {
  String line = Serial.readStringUntil('\n');
  if (line.startsWith("Content-Length: ")) {
    sscanf(line.c_str() + 16, "%d", &contentLength);
  } else if (line.startsWith("GET")) {
    method = GET;
    path = parsePath(line);
  } else if (line.startsWith("PUT")) {
    method = PUT;
    path = parsePath(line);
  } else if (line == "\r") {
    if (path.startsWith("/reset")) {
      sendResponse(200);
      setup();
    } else if (path.startsWith("/sensors") && method == GET) {
      getSensors();
    } else if (path.startsWith("/leds") && (method == GET)) {
      getLeds();
    } else if (path.startsWith("/leds") && (method == PUT)) {
      putLeds();
    } else if (path.startsWith("/settings") && (method == GET)) {
      getSettings();
    } else if (path.startsWith("/settings") && (method == PUT)) {
      putSettings();
    } else {
      sendResponse(404);
    }
    contentLength = 0;
  }
}

String parsePath(String line) {
  int firstPathChar = line.indexOf(' ') + 1;
  int nextPathChar = line.indexOf(' ', (unsigned int) firstPathChar);
  if (nextPathChar == -1) {
    nextPathChar = line.length();
  }
  return line.substring((unsigned int) firstPathChar, (unsigned int) nextPathChar);
}

String readContent() {
  Serial.readBytes(buffer, (size_t) min(contentLength, BUFFER_LENGTH));
  return String(buffer);
}

void sendResponse(int status, String content) {
  Serial.print("HTTP/1.1 ");
  Serial.println(status);
  Serial.println("Connection: close");
  Serial.print("Content-Length: ");
  Serial.println(content.length());
  Serial.println("Content-Type: application/json");
  Serial.println();
  if (content.length() > 0) {
    Serial.println(content);
  }
}

void sendResponse(int status, JsonObject &content) {
  String responseString;
  content.printTo(responseString);
  sendResponse(status, responseString);
}

void sendResponse(int status) {
  sendResponse(status, "");
}
