#include <SPI.h>
#include <WiFi.h>
#include "Credentials.h"

#include "TM1637.h"
#include "DHT.h"

// ================= PIN SETUP =================
#define BUZZER_PIN       36
#define CLK              39
#define DIO              38
#define TEMP_HUMI_PIN    24

bool sendingEnabled = true;
bool lastSW1 = HIGH;
bool lastSW3 = HIGH;

// ================= DEVICES =================
TM1637 tm1637(CLK, DIO);
DHT dht(TEMP_HUMI_PIN, DHT22);

// ================= WIFI STATE =================
bool hasPlayedConnectSound = false;
bool wasConnected = false;

// ================= BUZZER SONG =================
char notes[] = "ccggaagffeeddc ";
int beats[] = {1,1,1,1,1,1,2,1,1,1,1,1,1,2,4};
int tempo = 200;

// ================= TIMER =================
unsigned long lastTime = 0;
int seconds = 0;

// ================= DISPLAY MODE =================
// 0 = timer, 1 = temperature, 2 = humidity
int displayMode = 0;

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

// ================= DISPLAY CONTROLLER =================
void updateDisplay(int temp, int hum)
{
  if (displayMode == 0)
  {
    int d1 = (seconds / 1000) % 10;
    int d2 = (seconds / 100) % 10;
    int d3 = (seconds / 10) % 10;
    int d4 = seconds % 10;

    tm1637.display(0, d1);
    tm1637.display(1, d2);
    tm1637.display(2, d3);
    tm1637.display(3, d4);
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

// ================= SETUP =================
void setup()
{
  Serial.begin(9600);
  delay(500);

  pinMode(BUZZER_PIN, OUTPUT);

  // DISPLAY INIT
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL);
  tm1637.point(POINT_ON);

  // SENSOR INIT
  dht.begin();

  Serial.println("\n--- FULL IOT SYSTEM START ---");

  WiFi.begin(ssid, password);

  pinMode(73, INPUT_PULLUP);  // SW1
  pinMode(74, INPUT_PULLUP);  // SW3
  lastSW1 = digitalRead(73);
  lastSW3 = digitalRead(74);
}

// ================= LOOP =================
void loop()
{
  // ================= WIFI STATE =================
  bool connected = (WiFi.status() == WL_CONNECTED);

  if (connected && !wasConnected)
  {
    Serial.println("WiFi CONNECTED!");

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    // MAC
    byte mac[6];
    WiFi.macAddress(mac);

    Serial.print("MAC: ");
    for (int i = 0; i < 6; i++)
    {
      if (mac[i] < 16) Serial.print("0");
      Serial.print(mac[i], HEX);
      if (i < 5) Serial.print(":");
    }
    Serial.println();

    // buzzer ONCE
    if (!hasPlayedConnectSound)
    {
//      playConnectMelody();
      hasPlayedConnectSound = true;
    }
  }

  if (!connected && wasConnected)
  {
    Serial.println("WiFi DISCONNECTED!");
    hasPlayedConnectSound = false;
  }

  wasConnected = connected;

  bool sw1 = digitalRead(73);
  bool sw3 = digitalRead(74);
  
  // ================= SW1: toggle sending =================
  if (lastSW1 == HIGH && sw1 == LOW)
  {
    sendingEnabled = !sendingEnabled;
  
    Serial.print("Sending: ");
    Serial.println(sendingEnabled ? "ON" : "OFF");
  }
  
  // ================= SW3: reset system =================
  if (lastSW3 == HIGH && sw3 == LOW)
  {
    Serial.println("SYSTEM RESET");
  
    seconds = 0;
    lastTime = millis();
  
    hasPlayedConnectSound = false;
  }
  
  // update states
  lastSW1 = sw1;
  lastSW3 = sw3;

  // ================= TIMER (NON-BLOCKING) =================
  if (millis() - lastTime >= 1000)
  {
    lastTime += 1000;
    seconds++;

    if (seconds > 9999)
      seconds = 0;
  }

  // ================= SENSOR =================
  int temp = dht.readTemperature();
  int hum  = dht.readHumidity();

  if (!isnan(temp) && !isnan(hum))
  {
    Serial.print("Temp: ");
    Serial.print(temp);
    Serial.print(" Hum: ");
    Serial.println(hum);

    if (sendingEnabled)
    {
      sendData(temp, hum, WiFi.RSSI(), getMacString());
    }

    updateDisplay(temp, hum);
  }

  delay(100);
}

// ================= SEND DATA =================
void sendData(int temp, int hum, int rssi, String mac)
{
  WiFiClient client;

  const char* host = "192.168.0.57"; // CHANGE THIS
  const int port = 5000;

  if (client.connect(host, port))
  {
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

    Serial.println("Data sent!");
  }

  client.stop();
}

// ================= MAC STRING =================
String getMacString()
{
  byte mac[6];
  WiFi.macAddress(mac);

  String result = "";
  for (int i = 0; i < 6; i++)
  {
    if (mac[i] < 16) result += "0";
    result += String(mac[i], HEX);
    if (i < 5) result += ":";
  }
  return result;
}
