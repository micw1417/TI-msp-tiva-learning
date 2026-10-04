#include <SPI.h>
#include <WiFi.h>
#include "Credentials.h"
#include "Globals.h"

#include "TM1637.h"
#include "DHT.h"

// ===== PINS =====
#define BUZZER_PIN       36
#define CLK              39
#define DIO              38
#define TEMP_HUMI_PIN    24
#define SW1              73
#define SW3              74

// ===== DEVICES =====
TM1637 tm1637(CLK, DIO);
DHT dht(TEMP_HUMI_PIN, DHT22);

// ===== STATE =====
bool lastSW1 = HIGH;
bool lastSW3 = HIGH;

// ===== GLOBALS (DEFINED HERE) =====
int seconds = 0;
unsigned long lastSecondTick = 0;
bool sendingEnabled = true;
int displayMode = 0;

int currentTemp = 0;
int currentHum = 0;

// ===== TIMERS =====
unsigned long lastDHTRead = 0;
unsigned long lastSend = 0;


// ================= BUZZER SONG =================
char notes[] = "ccggaagffeeddc ";
int beats[] = {1,1,1,1,1,1,2,1,1,1,1,1,1,2,4};
int tempo = 200;
bool hasPlayedConnectSound = false;

// ================= NOTE FREQUENCY =================
int noteFreq(char note)
{
  switch(note)
  {
    case 'c': return 262;
    case 'd': return 294;
    case 'e': return 330;
    case 'f': return 349;
    case 'g': return 392;
    case 'a': return 440;
    case 'b': return 494;
    case 'C': return 523;
    default: return 0;
  }
}

// ================= BUZZER SOUND =================
void playConnectMelody()
{
  for (int i = 0; i < 15; i++)
  {
    if (notes[i] == ' ')
      delay(beats[i] * tempo);
    else
      tone(BUZZER_PIN, noteFreq(notes[i]), beats[i] * tempo);

    delay(tempo / 2);
  }
}


// ===== SETUP =====
void setup()
{
  Serial.begin(9600);
  Serial.println("BOOT START");
  delay(1000);
  Serial.println("BOOT OK");

//  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);

  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);
  tm1637.point(POINT_ON);

  dht.begin();

  lastSW1 = digitalRead(SW1);
  lastSW3 = digitalRead(SW3);

  WiFi.disconnect();
  delay(1000);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi...");
  
  // wait for WiFi link
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("\nWiFi link established");
  
  // NOW wait for IP
  while (WiFi.localIP() == IPAddress(0,0,0,0))
  {
    Serial.println("Waiting for IP...");
    delay(500);
  }
  
  Serial.println("WiFi READY");
  Serial.println(WiFi.localIP());
  // buzzer ONCE
  if (!hasPlayedConnectSound)
  {
    playConnectMelody();
    hasPlayedConnectSound = true;
  }

  // start server
  extern WiFiServer server;
  server.begin();
}

// ===== LOOP =====
void loop()
{
  // ===== WEB SERVER =====
  handleClient();

  // ===== BUTTONS =====
  bool sw1 = digitalRead(SW1);
  bool sw3 = digitalRead(SW3);

  if (lastSW1 == HIGH && sw1 == LOW)
  {
    sendingEnabled = !sendingEnabled;
    Serial.println(sendingEnabled ? "SENDING ON" : "SENDING OFF");
  }

  if (lastSW3 == HIGH && sw3 == LOW)
  {
    seconds = 0;
    lastSecondTick = millis();
    Serial.println("RESET");
  }

  lastSW1 = sw1;
  lastSW3 = sw3;

  // ===== TIMER =====
  if (millis() - lastSecondTick >= 1000)
  {
    lastSecondTick = millis();
    seconds++;
    if (seconds > 9999) seconds = 0;
  }

  // ===== SAFE DHT READ (EVERY 2s) =====
  if (millis() - lastDHTRead > 2000)
  {
    lastDHTRead = millis();

    int t = dht.readTemperature();
    int h = dht.readHumidity();

    if (!isnan(t) && !isnan(h))
    {
      currentTemp = t;
      currentHum = h;

      Serial.print("Temp: ");
      Serial.print(currentTemp);
      Serial.print(" Hum: ");
      Serial.println(currentHum);
    }
    else
    {
      Serial.println("Read fail");
    }
  }

  // ===== DISPLAY =====
  updateDisplay(currentTemp, currentHum);

  // ===== SEND DATA =====
  if (sendingEnabled && millis() - lastSend > 3000)
  {
    lastSend = millis();
    sendData(currentTemp, currentHum, WiFi.RSSI(), getMacString());
  }

  delay(50);
}

// ===== DISPLAY =====
void updateDisplay(int temp, int hum)
{
  if (displayMode == 0)
  {
    tm1637.display(0, (seconds / 1000) % 10);
    tm1637.display(1, (seconds / 100) % 10);
    tm1637.display(2, (seconds / 10) % 10);
    tm1637.display(3, seconds % 10);
  }
  else if (displayMode == 1)
  {
    tm1637.display(0, (temp / 10) % 10);
    tm1637.display(1, temp % 10);
    tm1637.display(2, 0);
    tm1637.display(3, 0);
  }
  else if (displayMode == 2)
  {
    tm1637.display(0, (hum / 10) % 10);
    tm1637.display(1, hum % 10);
    tm1637.display(2, 0);
    tm1637.display(3, 0);
  }
}

// ===== SEND DATA =====
void sendData(int temp, int hum, int rssi, String mac)
{
  WiFiClient client;
  client.setTimeout(1000);

  const char* host = "172.20.10.2";
  const int port = 5000;

  if (!client.connect(host, port))
  {
    Serial.println("CONN FAIL");
    
    return;
  }

  String json =
    String("{\"temp\":") + temp +
    ",\"humidity\":" + hum +
    ",\"rssi\":" + rssi +
    ",\"mac\":\"" + mac + "\"}";

  client.println("POST /data HTTP/1.1");
  client.println("Host: " + String(host));
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(json.length());
  client.println();
  client.println(json);

  Serial.println("Data sent");

  client.stop();
}

// ===== MAC =====
String getMacString()
{
  byte mac[6];
  WiFi.macAddress(mac);

  String result;
  for (int i = 0; i < 6; i++)
  {
    if (mac[i] < 16) result += "0";
    result += String(mac[i], HEX);
    if (i < 5) result += ":";
  }
  return result;
}
