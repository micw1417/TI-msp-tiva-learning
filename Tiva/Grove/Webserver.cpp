#include "WebServer.h"
#include "Globals.h"
#include <WiFi.h>

WiFiServer server(80);

void handleClient()
{
  WiFiClient client = server.available();
  if (!client) return;

  String req = client.readStringUntil('\n');
  req.trim();
  client.flush();

  Serial.println("Request: " + req);

  if (req.indexOf("GET /reset") >= 0)
  {
    seconds = 0;
    lastSecondTick  = millis();
    Serial.println("RESET triggered");
  }

  else if (req.indexOf("GET /pauseToggle") >= 0)
  {
    sendingEnabled = !sendingEnabled;
  }

  else if (req.indexOf("GET /mode") >= 0)
  {
    displayMode = (displayMode + 1) % 3;
  }

  // ===== STATUS ENDPOINT =====
  if (req.indexOf("GET /status") >= 0)
  {
    String json =
      String("{\"temp\":") + currentTemp +
      ",\"humidity\":" + currentHum +
      ",\"seconds\":" + seconds +
      ",\"sending\":" + (sendingEnabled ? "true" : "false") +
      "}";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println();
    client.println(json);
  }
  else
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println("<h1>OK</h1>");
  }

  client.stop();
}
