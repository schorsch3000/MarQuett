#include <LEDMatrixDriver.hpp>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "font.h"
#include "local_config.h"

#ifndef TOPICROOT
#define TOPICROOT "ledMatrix"
#endif

uint8_t text[MAX_TEXT_LENGTH];
LEDMatrixDriver led(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN, 0);
uint16_t textIndex = 0;
uint8_t colIndex = 0;
uint16_t scrollWhitespace = 0;
uint64_t marqueeDelayTimestamp = 0;
uint64_t marqueeBlinkTimestamp;
uint16_t blinkDelay = 0;
char devname[20];

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.setBufferSize(MAX_TEXT_LENGTH);
  for (int i = 0; i < sizeof(text); i++) {
    text[i] = initialText[i];
  }
  led.setIntensity(0);
  led.setEnabled(true);
  calculate_font_index();
  if (do_publishes)
    client.publish((((String)TOPICROOT "/" + devname + "/status").c_str()), "startup");
}

void loop()
{
  if (blinkDelay) {
    if (marqueeBlinkTimestamp > millis()) {
      marqueeBlinkTimestamp > millis();
    }
    if (marqueeBlinkTimestamp + blinkDelay < millis()) {
      led.setEnabled(false);
      delay(1);
      marqueeBlinkTimestamp = millis();
    } else if (marqueeBlinkTimestamp + blinkDelay / 2 < millis()) {
      led.setEnabled(true);
      delay(1);
    }
  }
  loop_matrix();
  if (!client.connected()) {
    reconnect();
    if (do_publishes)
      client.publish((((String)TOPICROOT "/" + devname + "/status").c_str()), "reconnect");
  }
  client.loop();
}


void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.print("WiFi connected");
  Serial.print(" - IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print(" - MAC address: ");
  Serial.println(WiFi.macAddress());
  byte mac[6];
  WiFi.macAddress(mac);
  snprintf(devname, sizeof(devname), "MarQueTTino/%02X%02X%02X", mac[3], mac[4], mac[5]);
  Serial.println((String)"This device is called '" + devname + "'.");
}

void printHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
  char tmp[16];
  for (int i = 0; i < length; i++) {
    sprintf(tmp, "0x%.2X", data[i]);
    Serial.print(tmp); Serial.print(" ");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print((String)"MQTT in: " + topic + "\t = [");
  for (int i = 0; i < length; i++)  Serial.print((char)(payload[i]));
  Serial.println("]");

  char* command = topic + String(topic).lastIndexOf("/") + 1;

  if (!strcmp(command, "intensity")) {
    int intensity = 0;
    for (int i = 0 ; i < length;  i++) {
      intensity *= 10;
      intensity += payload[i] - '0';
    }
    intensity = max(0, min(15, intensity));
    led.setIntensity(intensity);
    return;
  }

  if (!strcmp(command, "delay")) {
    scrollDelay = 0;
    for (int i = 0 ; i < length;  i++) {
      scrollDelay *= 10;
      scrollDelay += payload[i] - '0';
    }
    if (scrollDelay == 0) {
      textIndex = 0;
    } else if (scrollDelay < 1) {
      scrollDelay = 1;
    }
    if (scrollDelay > 10000) {
      scrollDelay = 10000;
    }

    return;
  }

  if (!strcmp(command, "blink")) {
    blinkDelay = 0;
    for (int i = 0 ; i < length;  i++) {
      blinkDelay *= 10;
      blinkDelay += payload[i] - '0';
    }
    if (blinkDelay < 0) {
      blinkDelay = 0;
    }
    if (blinkDelay > 10000) {
      blinkDelay = 10000;
    }
    if (!blinkDelay) {
      led.setEnabled(true);
    } else {
      marqueeBlinkTimestamp = millis();
    }

    return;
  }


  if (!strcmp(command, "enable")) {
    led.setEnabled(payload[0] == '1');
  }


  if (!strcmp(command, "text")) {
    const bool pr = 1;                                // set to 1 for debug prints
    if (pr) printHex8(payload, length);
    text[0] = ' ';
    for (int i = 0 ; i < 4096; i++) {
      text[i] = 0;
    }
    char tmp[16];
    int i = 0, j = 0;
    while (i < length) {
      uint8_t b = payload[i++];
      sprintf(tmp, "0x%.2X = '%c' -> ", b, b);
      if (pr) Serial.print(tmp);
      if ((b & 0b10000000) == 0) {                    // 7-bit ASCII
        if (pr) Serial.println("ASCII");
        text[j++] = b;

      } else if ((b & 0b11100000) == 0b11000000) {    // UTF-8 2-byte sequence: starts with 0b110xxxxx
        if (pr) Serial.println("UTF-8 (2)");
        if (b == 0xc3) {                              // UTF-8 1st byte
          text[j++] = payload[i++] + 64;              // map 2nd byte to Latin-1 (simplified)
        } else if (b == 0xc2) {
          if (i < length && payload[i] == 0xA7) {         // § = section sign
            text[j++] = 0xA7;
          } else if (i < length && payload[i] == 0xB0) {  // ° = degrees sign
            text[j++] = 0xB0;
          } else if (i < length && payload[i] == 0xB5) {  // µ = mu
            text[j++] = 0xB5;
          } else {
            // unknown
            text[j++] = 0x7f;                         // add checkerboard pattern
          }
          i += 1;
        } else {
          // unknown
          text[j++] = 0x7f;                           // add checkerboard pattern
          i += 1;
        }
      } else if ((b & 0b111100000) == 0b11100000) {   // UTF-8 3-byte sequence: starts with 0b1110xxxx
        if (pr) Serial.println("UTF-8 (3)");
        if (i + 1 < length && b == 0xE2) {
          if (payload[i] == 0x82 && payload[i + 1] == 0xAC) {
            text[j++] = 0x80;                           // € = euro sign
          } else if (payload[i] == 0x80 && payload[i + 1] == 0x93) {
            text[j++] = 0x96;                           // – = en dash
          } else if (payload[i] == 0x80 && payload[i + 1] == 0x94) {
            text[j++] = 0x97;                           // — = em dash
          } else if (payload[i] == 0x80 && payload[i + 1] == 0xA6) {
            text[j++] = 0x85;                           // … = ellipsis
          } else {
            // unknown
            text[j++] = 0x7f;                         // add checkerboard pattern
          }
          i += 2;
        } else {
          // unknown, skip remaining 2 bytes
          i += 2;
          text[j++] = 0x7f;                           // add checkerboard pattern
          text[j++] = 0x7f;                           // add checkerboard pattern
        }
      } else if ((b & 0b111110000) == 0b11110000) {   // UTF-8 4-byte sequence_ starts with 0b11110xxx
        if (pr) Serial.println("UTF-8 (4)");
        // unknown, skip remaining 3 bytes
        i += 3;
        text[j++] = 0x7f;                             // add checkerboard pattern
        text[j++] = 0x7f;                             // add checkerboard pattern
        text[j++] = 0x7f;                             // add checkerboard pattern
      } else {                                        // unsupported (illegal) UTF-8 sequence
        if (pr) Serial.print("UTF-8 (n) ");
        while ((payload[i] & 0b10000000) && i < length) {
          if (pr) Serial.print("+");
          i++;                                        // skip non-ASCII characters
          text[j++] = 0x7f;                           // add checkerboard pattern
        }
        if (pr) Serial.println();
      }
    }
    for (int i = 0; i < j; i++) {
      uint8_t asc = text[i] - 32;
      uint16_t idx = pgm_read_word(&(font_index[asc]));
      uint8_t w = pgm_read_byte(&(font[idx]));
      if (w == 0) {
        // character is NOT defined, replace with 0x7f = checkerboard pattern
        text[i] = 0x7f;
      }
    }
    if (pr) Serial.print((String)"=> Text (" + j + " bytes): ");
    if (pr) printHex8(text, j + 1);

    textIndex = 0;
    colIndex = 0;
    marqueeDelayTimestamp = 0;
    scrollWhitespace = 0;
    led.clear();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password, TOPICROOT "/status", 1, true, "Offline")) {
      Serial.println("connected");
      client.subscribe(TOPICROOT "/enable");
      client.subscribe(((String)TOPICROOT "/" + devname + "/enable").c_str());
      client.subscribe(TOPICROOT "/intensity");
      client.subscribe(((String)TOPICROOT "/" + devname + "/intensity").c_str());
      client.subscribe(TOPICROOT "/delay");
      client.subscribe(((String)TOPICROOT "/" + devname + "/delay").c_str());
      client.subscribe(TOPICROOT "/text");
      client.subscribe(((String)TOPICROOT "/" + devname + "/text").c_str());
      client.subscribe(TOPICROOT "/blink");
      client.subscribe(((String)TOPICROOT "/" + devname + "/blink").c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

const uint16_t LEDMATRIX_WIDTH    = LEDMATRIX_SEGMENTS * 8;

void nextChar()
{
  if (text[++textIndex] == '\0')
  {
    textIndex = 0;
    scrollWhitespace = LEDMATRIX_WIDTH; // start over with empty display
    if (scrollDelay && do_publishes)
      client.publish((((String)TOPICROOT "/" + devname + "/status").c_str()), "repeat");
  }
}

void nextCol(uint8_t w)
{
  if (++colIndex == w)
  {
    scrollWhitespace = 2;
    colIndex = 0;
    nextChar();
  }
}

void writeCol()
{
  if (scrollWhitespace > 0)
  {
    scrollWhitespace--;
    return;
  }
  uint8_t asc = text[textIndex] - 32;
  uint16_t idx = pgm_read_word(&(font_index[asc]));
  uint8_t w = pgm_read_byte(&(font[idx]));
  uint8_t col = pgm_read_byte(&(font[colIndex + idx + 1]));
  led.setColumn(LEDMATRIX_WIDTH - 1, col);
  nextCol(w);
}

void marquee()
{
  if (millis() < 1)
    marqueeDelayTimestamp = 0;
  if (millis() < marqueeDelayTimestamp)
    return;
  marqueeDelayTimestamp = millis() + scrollDelay;
  led.scroll(LEDMatrixDriver::scrollDirection::scrollLeft);
  writeCol();
  led.display();
}

void loop_matrix()
{
  if (scrollDelay) {
    marquee();
  } else {
    if (textIndex == 0) {   // start writing text to display (e.g. after text was changed)
      Serial.println("display static text");
      colIndex = 0;
      marqueeDelayTimestamp = 0;
      scrollWhitespace = 0;
      led.clear();
      uint8_t displayColumn = 0; // start left
      while (displayColumn < LEDMATRIX_WIDTH) {
        // write one column
        uint8_t asc = text[textIndex] - 32;
        uint16_t idx = pgm_read_word(&(font_index[asc]));
        uint8_t w = pgm_read_byte(&(font[idx]));
        uint8_t col = pgm_read_byte(&(font[colIndex + idx + 1]));
        led.setColumn(displayColumn, col);
        led.display();
        displayColumn++;
        if (++colIndex == w) {
          displayColumn += 1;
          colIndex = 0;
          if (text[++textIndex] == '\0') {
            return; // done
            textIndex = 0;
          }
        }
      }
    }
  }
}
