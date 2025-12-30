#!/usr/bin/env python3
"""
rtl_sdr -f 433920000 -s 250000 -g 40 - | python3 blind_live.py
"""

import sys
import numpy as np

THRESHOLD = 50
SAMPLE_RATE = int(2.5e5)


def decode_frame(edges, dirs):
    """Decode Manchester from edges. edges = sample positions, dirs = R/F"""
    if len(edges) < 10:
        return None

    intervals = np.diff(edges)
    sorted_int = np.sort(intervals)
    H = np.median(sorted_int[: len(sorted_int) // 2 + 1])
    thresh = H * 1.5

    bits = []
    is_midbit = True

    for i in range(len(dirs)):
        if is_midbit:
            bits.append("1" if dirs[i] == "R" else "0")
        if i < len(intervals):
            if intervals[i] > thresh:
                is_midbit = True
            else:
                is_midbit = not is_midbit

    if len(bits) < 64:
        return None

    b = [int("".join(bits[i : i + 8]), 2) for i in range(0, 64, 8)]
    chk_ok = ((sum(b[1:7]) + 0x2B) & 0xFF) == b[7]

    cmd_names = {0x01: "PAUSE", 0x02: "UP", 0x04: "DOWN", 0x08: "PAIR"}
    return {
        "cmd": cmd_names.get(b[1], f"0x{b[1]:02X}"),
        "blind_id": b[4] & 0x0F,
        "b2": b[2],
        "counter": b[3],
        "remote_id": ((b[4] & 0xF0) << 12) | (b[5] << 8) | b[6],
        "chk": chk_ok,
        "raw": " ".join(f"{x:02X}" for x in b),
    }


prev_raw = ""
repeat_count = 0
last_edge = 0
last_level = 0
collecting = False
edges = []
dirs = []
sample_pos = 0


def try_decode():
    global prev_raw, repeat_count, collecting, edges, dirs
    if len(edges) > 3:
        p = decode_frame(np.array(edges[1:]), dirs[1:])
        if p:
            if p["raw"] == prev_raw:
                repeat_count += 1
            else:
                repeat_count = 1
                prev_raw = p["raw"]
            c = "+" if p["chk"] else "-"
            r = f"x{repeat_count}" if repeat_count > 1 else "  "
            print(
                f"{p['cmd']:5s} blind={p['blind_id']:2d} b2={p['b2']:02X} ctr={p['counter']:02X} remote={p['remote_id']:05X} | {p['raw']} {c} {r}"
            )
            sys.stdout.flush()
    collecting = False
    edges = []
    dirs = []


while True:
    data = sys.stdin.buffer.read(8192)
    if not data:
        if collecting:
            try_decode()
        break

    raw = np.frombuffer(data, dtype=np.uint8)
    inph = raw[0::2].astype(np.float32) - 127.5
    quad = raw[1::2].astype(np.float32) - 127.5
    mag = np.sqrt(inph * inph + quad * quad)
    lvl = (mag > THRESHOLD).astype(np.uint8)

    for i in range(len(lvl)):
        pos = sample_pos + i

        # Check for timeout: if collecting and no edge for 7.5 ms, decode
        if collecting and (pos - last_edge) > 0.0075 * SAMPLE_RATE:
            try_decode()

        if lvl[i] != last_level:
            gap = pos - last_edge
            d = "R" if lvl[i] == 1 else "F"

            if d == "F" and gap > 0.005 * SAMPLE_RATE:
                if collecting:
                    try_decode()
                collecting = True
                edges = [pos]
                dirs = [d]
            elif collecting:
                edges.append(pos)
                dirs.append(d)

            last_edge = pos
            last_level = lvl[i]

    sample_pos += len(lvl)
