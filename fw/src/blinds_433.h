#pragma once

#include "util.h"
#include <Arduino.h>

static constexpr u32 kSyncHalfUs = 295;
static constexpr u32 kHalfBitUs = 640;
static constexpr u32 kPreambleUs = 6800;
static constexpr u32 kFrameGapUs = 25000;

class Blinds433 {
public:
  Blinds433(u8 tx_pin, u32 remote) : pin(tx_pin), remote_id(remote) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);

    for (u32 i = 0; i < 16; ++i) {
      counters[i] = 0x0410;
    }
  }

  void send(u8 cmd, u8 blind_id, i32 repeats = 6) {
    u8 d[8];
    d[0] = 0x81;
    d[1] = cmd;

    d[2] = counters[blind_id] >> 8;
    d[3] = counters[blind_id] & 0xFF;
    ++counters[blind_id];

    d[4] = ((remote_id >> 12) & 0xF0) | (blind_id & 0x0F);
    d[5] = (remote_id >> 8) & 0xFF;
    d[6] = remote_id & 0xFF;

    d[7] = (d[1] + d[2] + d[3] + d[4] + d[5] + d[6] + 0x2B) & 0xFF;
    for (int i = 0; i < repeats; i++) {
      send_frame(d, i);
      delayMicroseconds(kFrameGapUs);
    }
  }

  void up(u8 blind) { send(0x02, blind); }
  void down(u8 blind) { send(0x04, blind); }
  void pause(u8 blind) { send(0x01, blind); }
  void all_up() { send(0x02, 0); }
  void all_down() { send(0x04, 0); }
  void all_pause() { send(0x01, 0); }

private:
  void send_bit(u8 b) {
    if (b) { // 1 = LOW-HIGH
      digitalWrite(pin, LOW);
      delayMicroseconds(kHalfBitUs);
      digitalWrite(pin, HIGH);
      delayMicroseconds(kHalfBitUs);
    } else { // 0 = HIGH-LOW
      digitalWrite(pin, HIGH);
      delayMicroseconds(kHalfBitUs);
      digitalWrite(pin, LOW);
      delayMicroseconds(kHalfBitUs);
    }
  }

  void send_frame(u8 *data, const u32 frame_num) {
    const u32 sync_count = frame_num == 0 ? 434 : 59;
    for (u32 i = 0; i < sync_count; ++i) {
      digitalWrite(pin, HIGH);
      delayMicroseconds(kSyncHalfUs);
      digitalWrite(pin, LOW);
      delayMicroseconds(kSyncHalfUs);
    }

    digitalWrite(pin, HIGH);
    delayMicroseconds(kPreambleUs);

    digitalWrite(pin, LOW);
    delayMicroseconds(kHalfBitUs);

    for (int i = 0; i < 8; i++)
      for (int j = 7; j >= 0; j--)
        send_bit((data[i] >> j) & 1);

    digitalWrite(pin, LOW);
  }

  u8 pin;
  u32 remote_id;

  u16 counters[16];
};