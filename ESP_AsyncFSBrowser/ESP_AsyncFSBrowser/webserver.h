void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      Serial.printf("[HEX]= %x \r\n", info->len);
      Serial.printf("size = %d \r\n", info->len);
      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }

      Serial.printf("MESSAGE => %s\n", msg.c_str());
      String retText = "EMPTY";
      bool validMessage = 0;
      if (msg.startsWith("dhtType=")) {
        String value = msg.substring(8);
        if (value == "11" || value == "22") {
          validMessage = 1;
          dhtType = value.toInt();
          saveConfig();
          retText = String("OK! dhtType="+ value);
        }
        else {
        }
      }
      else if (msg.startsWith("myName=")) {
        validMessage = 1;
        String value = msg.substring(7);
        strcpy(myName, value.c_str());
        saveConfig();
        retText = String("OK! myName="+ value);
      }
      else if (msg == "$REBOOT") {
        validMessage = 1;
        retText = String("OK! $REBOOT");
      }
      else {
        Serial.printf("INVALID: %s \r\n", msg.c_str());
      }

      if(info->opcode == WS_TEXT)
        if (validMessage) {
          client->text(retText);
          delay(10);
          if (retText == "OK! $REBOOT") {
            client->text("BYE!");
            WiFi.disconnect();
            delay(200);
            ESP.reset();
          }

        }
        else {
          client->text(String("INVALID: ") + msg);
        }
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
    }
}
void setupWebServer() {
    ws->onEvent(onWsEvent);
    server->addHandler(ws);
    events->onConnect([](AsyncEventSourceClient *client){
      client->send("hello!",NULL,millis(),1000);
    });
    server->addHandler(events);
    server->addHandler(new SPIFFSEditor(http_username.c_str() ,http_password.c_str()));
    server->on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });
    server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");

    server->onNotFound([](AsyncWebServerRequest *request){
      Serial.printf("NOT_FOUND: ");
      if(request->method() == HTTP_GET)
        Serial.printf("GET");
      else if(request->method() == HTTP_POST)
        Serial.printf("POST");
      else if(request->method() == HTTP_DELETE)
        Serial.printf("DELETE");
      else if(request->method() == HTTP_PUT)
        Serial.printf("PUT");
      else if(request->method() == HTTP_PATCH)
        Serial.printf("PATCH");
      else if(request->method() == HTTP_HEAD)
        Serial.printf("HEAD");
      else if(request->method() == HTTP_OPTIONS)
        Serial.printf("OPTIONS");
      else
        Serial.printf("UNKNOWN");
      Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

      if(request->contentLength()){
        Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
        Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
      }

      int headers = request->headers();
      int i;
      for(i=0;i<headers;i++){
        AsyncWebHeader* h = request->getHeader(i);
        Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
      }

      int params = request->params();
      for(i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isFile()){
          Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } else if(p->isPost()){
          Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } else {
          Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }

      request->send(404);
    });
    server->onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
      if(!index)
        Serial.printf("UploadStart: %s\n", filename.c_str());
      Serial.printf("%s", (const char*)data);
      if(final)
        Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
    });
    server->onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if(!index)
        Serial.printf("BodyStart: %u\n", total);
      Serial.printf("%s", (const char*)data);
      if(index + len == total)
        Serial.printf("BodyEnd: %u\n", total);
    });
    server->begin();
    Serial.println("Starting webserver-..");
}
