/*
 QIPIQ Mini ESP32-S3 → QIPIQ Server → OpenAI (USB serial proof-of-connection)
 Does NOT capture microphone or play sound on physical board yet.
 Must provision VALID server root certificate; insecure TLS prohibited.
 Tested logically only, not compiled/flashed on real Waveshare PCB.
 Libraries: Arduino-ESP32 3.x, ArduinoJson 7.x.
*/
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "wifi_config.h"

static const uint32_t CONNECT_TIMEOUT = 20000;
static String typedLine;

bool networkReady(){
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.begin(QIPIQ_WIFI_SSID, QIPIQ_WIFI_PASSWORD);
  Serial.print("Connecting Wi-Fi");
  uint32_t since=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-since < CONNECT_TIMEOUT){ delay(400);Serial.print("."); }
  Serial.println();
  return WiFi.status()==WL_CONNECTED;
}

void askQipiq(const String& question){
  if(!question.length()||question.length()>420){Serial.println("Қысқа сұрақ енгізіңіз (420 таңбаға дейін)");return;}
  if(!String(QIPIQ_GATEWAY_URL).startsWith("https://")){
    Serial.println("Қауіпсіздік: HTTPS шлюз мекенжайын енгізіңіз");return;
  }
  if(strlen(QIPIQ_ROOT_CA_PEM)<250){
    Serial.println("Қауіпсіздік: сервердің нақты root CA сертификатын wifi_config.h ішіне салыңыз");return;
  }
  if(!networkReady()){Serial.println("Wi-Fi қосылмады");return;}

  WiFiClientSecure tls;
  tls.setCACert(QIPIQ_ROOT_CA_PEM); // Real CA cert required. Never use setInsecure().
  HTTPClient http;
  String endpoint=String(QIPIQ_GATEWAY_URL)+"/v1/chat";
  if(!http.begin(tls,endpoint)) {Serial.println("HTTPS қосылу қатесі");return;}
  http.setTimeout(23000);
  http.addHeader("Content-Type","application/json");
  http.addHeader("X-Device-Token",QIPIQ_DEVICE_TOKEN);
  JsonDocument req;
  req["message"]=question;
  req["history"].to<JsonArray>();
  String payload;
  serializeJson(req,payload);
  Serial.println("QIPIQ AI жауап беруде...");
  int status=http.POST(payload);
  if(status==200){
    String body=http.getString();
    JsonDocument result;
    DeserializationError err=deserializeJson(result,body);
    if(err){Serial.println("JSON қатесі");}
    else{String reply=result["reply"].as<String>();Serial.print("QIPIQ: ");Serial.println(reply);}
  } else {
    Serial.print("HTTP қате коды: ");Serial.println(status);
    if(status==401)Serial.println("DEVICE_TOKEN қате");
    if(status==429)Serial.println("Шектеу асып кетті");
  }
  http.end();
}

void setup(){
  Serial.begin(115200);
  delay(1800);
  Serial.println("QIPIQ Mini Cloud Serial Test v1");
  Serial.println("Бұл тест USB Serial Monitor арқылы ғана сөйлеседі.");
  Serial.println("Бағдарлама жасырын OpenAI API кілтін қамтымайды.");
  Serial.println("Сұрағыңызды теріп Enter басыңыз.");
}
void loop(){
  while(Serial.available()){
    char c=(char)Serial.read();
    if(c=='\n'){
      typedLine.trim();
      if(typedLine.length()) askQipiq(typedLine);
      typedLine="";
    } else if(c!='\r' && typedLine.length()<500){typedLine+=c;}
  }
  delay(5);
}
