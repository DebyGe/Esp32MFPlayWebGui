#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// === CONFIG WiFi ===
const char* ssid = "Vodafone-A55435553";         
const char* password = "Ln33rLf4Ryab6cdY";

// 🌐 Web server
ESP8266WebServer server(80);

SoftwareSerial mySoftwareSerial(D5, D6); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

int currentVolume = 20;  // Volume iniziale
int currentTrack = 1;        // Traccia attuale
bool isLooping = false;      // Stato loop (opzionale, se vuoi usarlo visivamente)

void handleRoot() {
  // 🌍 HTML semplice
  String htmlPage = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>DFPlayer Control</title>
    <style>
      body { font-family: sans-serif; text-align: center; padding-top: 30px; }
      button { padding: 15px 30px; margin: 10px; font-size: 18px; }
    </style>
  </head>
  <body>
    <h2>Controllo DFPlayer Mini</h2>
    <h2>CurrentTrak: %TRAK% </h2>
    
    <button onclick="location.href='/prev'">Prev</button>
    <button onclick="location.href='/play'">Play</button>
    <button onclick="location.href='/next'">Next</button>
    <button onclick="location.href='/stop'">Stop</button>
    <button onclick="location.href='/loop'">Loop</button>

    <br><br>
    <form action="/volume" method="get">
        <label for="volume">Volume: <span id='valLabel'>%VOLUME%</span></label><br>
        <input type="range" id="volume" name="val" min="0" max="30" value="%VOLUME%" oninput="valLabel.innerText=this.value" onchange="this.form.submit()">
      </form>

  </body>
  </html>
  )rawliteral";
  
  htmlPage.replace("%TRAK%", String(currentTrack));
  htmlPage.replace("%VOLUME%", String(currentVolume));
  server.send(200, "text/html", htmlPage);
}

void handlePlay() {
  myDFPlayer.play(currentTrack); // Riproduci 0001.mp3
  server.sendHeader("Location", "/");
  server.send(303); // Redirect alla home
}

void handleNext() {
  myDFPlayer.next();
  currentTrack++;
  server.sendHeader("Location", "/");
  server.send(303); // Redirect alla home
}

void handlePrev() {
  myDFPlayer.previous();
  currentTrack--;
  server.sendHeader("Location", "/");
  server.send(303); // Redirect alla home
}

void handleStop() {
  myDFPlayer.stop();
  server.sendHeader("Location", "/");
  server.send(303); // Redirect alla home
}

void handleVolume() {
  if (server.hasArg("val")) {
    int vol = server.arg("val").toInt();
    vol = constrain(vol, 0, 30);
    myDFPlayer.volume(vol);
    currentVolume = vol;
    Serial.print("Volume impostato a: ");
    Serial.println(vol);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleLoop() {
  myDFPlayer.loop(currentTrack);
  isLooping = true;
  Serial.print("Looping traccia ");
  Serial.println(currentTrack);
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup()
{
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));
  
  currentVolume = 20;

  myDFPlayer.volume(currentVolume);  //Set volume value. From 0 to 30
  myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);

  // Connessione Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connessione a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnesso! IP:");
  Serial.println(WiFi.localIP());

  // Rotte web
  server.on("/", handleRoot);
  server.on("/prev", handlePrev);
  server.on("/play", handlePlay);
  server.on("/next", handleNext);
  server.on("/stop", handleStop);
  server.on("/loop", handleLoop);
  server.on("/volume", handleVolume);

  server.begin();
  Serial.println("Web server avviato.");

  //myDFPlayer.play(1);  //Play the first mp3
}

void loop()
{
  server.handleClient();

  // static unsigned long timer = millis();
  
  // if (millis() - timer > 3000) {
  //   timer = millis();
  //   myDFPlayer.next();  //Play next mp3 every 3 second.
  // }
  
  // if (myDFPlayer.available()) {
  //   printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  // }
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  
}
