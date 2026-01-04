#include "blinds_433.h"
#include "config.h"
#include "embedded_files.h"
#include "util.h"

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

ESP8266WebServer server(80);

static i8 hex_val(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

static u16 parse_hex4(const char *s) {
  return (hex_val(s[0]) << 12) | (hex_val(s[1]) << 8) | (hex_val(s[2]) << 4) |
         hex_val(s[3]);
}

static bool is_valid_name_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_' || c == '-' || c == ' ';
}

void handle_root() { server.send(200, "text/html", index_html); }

void handle_list_get() {
  string<2048> json;
  u32 pos = 0;
  json[pos++] = '[';
  bool first = true;

  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    File f = dir.openFile("r");
    if (UNLIKELY(!f))
      continue;
    defer(f.close());

    u16 id;
    if (f.read((u8 *)&id, 2) != 2)
      continue;

    if (!first)
      json[pos++] = ',';
    first = false;

    String fnameStr = dir.fileName();
    const char *fname = fnameStr.c_str();
    if (fname[0] == '/')
      fname++;

    auto entry = sprintn(64, "[\"%s\",\"%04X\"]", fname, id);
    u32 len = __builtin_strlen(entry.str());
    __builtin_memcpy(&json[pos], entry.str(), len);
    pos += len;
  }

  json[pos++] = ']';
  json[pos] = '\0';

  server.send(200, "application/json", json.str());
}

void handle_list_post() {
  string<64> body = server.arg("plain").c_str();
  u32 len = __builtin_strlen(body.str());

  i32 comma = -1;
  u32 comma_count = 0;
  for (u32 i = 0; i < len; i++) {
    if (body[i] == ',') {
      comma = i;
      comma_count++;
    }
  }

  if (comma_count != 1 || comma == 0 || comma == (i32)(len - 1)) {
    server.send(400, "text/plain", "invalid format");
    return;
  }

  u32 name_len = comma;
  u32 id_len = len - comma - 1;

  if (name_len > 16 || name_len == 0) {
    server.send(400, "text/plain", "name must be 1-16 chars");
    return;
  }

  for (u32 i = 0; i < name_len; i++) {
    if (UNLIKELY(!is_valid_name_char(body[i]))) {
      server.send(400, "text/plain", "name contains invalid char");
      return;
    }
  }

  if (id_len != 4) {
    server.send(400, "text/plain", "id must be 4 hex chars");
    return;
  }

  for (u32 i = 0; i < 4; i++) {
    if (UNLIKELY(hex_val(body[comma + 1 + i]) < 0)) {
      server.send(400, "text/plain", "id must be hex");
      return;
    }
  }

  u16 id = parse_hex4(&body[comma + 1]);

  string<32> name;
  __builtin_memcpy(&name[0], body.str(), name_len);
  name[name_len] = '\0';

  auto path = sprintn(32, "/%s", name.str());
  File f = LittleFS.open(path.str(), "w");
  if (UNLIKELY(!f)) {
    server.send(500, "text/plain", "fs error");
    return;
  }
  defer(f.close());

  f.write((const u8 *)&id, 2);
  server.send(200, "text/plain", "ok");
}

void handle_list_delete() {
  string<32> name = server.arg("plain").c_str();
  u32 len = __builtin_strlen(name.str());

  while (len > 0 && (name[len - 1] == ' ' || name[len - 1] == '\n' ||
                     name[len - 1] == '\r' || name[len - 1] == '\t')) {
    name[--len] = '\0';
  }

  if (len == 0 || len > 16) {
    server.send(400, "text/plain", "invalid name");
    return;
  }

  for (u32 i = 0; i < len; i++) {
    if (UNLIKELY(!is_valid_name_char(name[i]))) {
      server.send(400, "text/plain", "invalid name");
      return;
    }
  }

  auto path = sprintn(32, "/%s", name.str());
  if (!LittleFS.exists(path.str())) {
    server.send(404, "text/plain", "not found");
    return;
  }

  LittleFS.remove(path.str());
  server.send(200, "text/plain", "ok");
}

static constexpr u8 kDataPin = 4;
Blinds433 blinds(kDataPin, 0xF1331);

void setup() {
  Serial.begin(9600);
  pinMode(kDataPin, OUTPUT);
  Serial.println("Initialized.");

  // Serial.begin(9600);
  // LittleFS.begin();

  // WiFi.begin(kWifiSsid, kWifiPassword);
  // while (!WiFi.isConnected()) {
  //   Serial.println("Connecting...");
  //   delay(500);
  // }

  // Serial.print("IP: ");
  // Serial.println(WiFi.localIP());

  // server.on("/", handle_root);
  // server.on("/list", HTTP_GET, handle_list_get);
  // server.on("/list", HTTP_POST, handle_list_post);
  // server.on("/list", HTTP_DELETE, handle_list_delete);
  // server.begin();
}

void loop() {
  Serial.println("sending blinds down");
  blinds.all_down();
  delay(2000);
  // Serial.println("pausing all blinds");
  // blinds.all_pause();
  // delay(1000);
  // Serial.println("sending all blinds up");
  // blinds.all_up();
  // delay(1000);
  // server.handleClient();
}