#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include "OV2640.h"
#include "SimStreamer.h"
#include "OV2640Streamer.h"
#include "CRtspSession.h"

char *ssid = "YOURSSID";         // Put your SSID here
char *password = "YOURPASSWORD"; // Put your PASSWORD here

WiFiServer rtspServer(8554);
OV2640 cam;
CStreamer *streamer;

//WiFi Connection
void WifiConnecte()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected");
  Serial.print("IP Address:");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(115200);
  //UXGA(1600x1200),SXGA(1280x1024),XGA(1024x768),SVGA(800x600),VGA(640x480),CIF(400x296),QVGA(320x240),HQVGA(240x176),QQVGA(160x120)
  esp32cam_aithinker_config.frame_size = FRAMESIZE_SVGA;
  esp32cam_aithinker_config.jpeg_quality = 10;
  cam.init(esp32cam_aithinker_config);
  //Start WiFi connection
  WifiConnecte();

  rtspServer.begin();
  streamer = new OV2640Streamer(cam);
  Serial.println("WiFi connected");
  Serial.print("Use RTSP:'");
  Serial.print(WiFi.localIP());
  Serial.println("', URL:'/mjpeg/1' and port:554 to start rtsp stream");
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WifiConnecte();
  }

  uint32_t msecPerFrame = 100;
  static uint32_t lastimage = millis();
  // If we have an active client connection, just service that until gone
  streamer->handleRequests(0); // we don't use a timeout here,
  // instead we send only if we have new enough frames
  uint32_t now = millis();
  if (streamer->anySessions())
  {
    if (now > lastimage + msecPerFrame || now < lastimage)
    { // handle clock rollover
      streamer->streamImage(now);
      lastimage = now;

      // check if we are overrunning our max frame rate
      now = millis();
      if (now > lastimage + msecPerFrame)
      {
        printf("warning exceeding max frame rate of %d ms\n", now - lastimage);
      }
    }
  }
  WiFiClient rtspClient = rtspServer.accept();
  if (rtspClient)
  {
    Serial.print("client: ");
    Serial.print(rtspClient.remoteIP());
    Serial.println();
    streamer->addSession(rtspClient);
  }
}
