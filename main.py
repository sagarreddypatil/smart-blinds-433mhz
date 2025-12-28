#!/usr/bin/env python3
"""
rtl_sdr -f 433920000 -s 2000000 -g 40 - | uv run main.py
"""

import sys
import numpy as np

sr = int(2e6)
chunk_samples = sr // 2  # 0.5 sec chunks


def decode_manchester(edges, directions):
    if len(edges) < 3:
        return ""

    intervals = np.diff(edges)
    sorted_int = np.sort(intervals)
    H = np.median(sorted_int[: len(sorted_int) // 2 + 1])
    threshold = H * 1.5

    bits = []
    is_midbit = True

    for i in range(len(directions)):
        if is_midbit:
            bits.append("1" if directions[i] == "R" else "0")

        if i < len(intervals):
            if intervals[i] > threshold:
                is_midbit = True
            else:
                is_midbit = not is_midbit

    return "".join(bits)


def process_chunk(iq, threshold=50):
    mag = np.abs(iq)
    ook = (mag > threshold).astype(np.uint8)

    diff = np.diff(ook.astype(np.int8))
    rising = np.where(diff == 1)[0]
    falling = np.where(diff == -1)[0]

    if len(rising) == 0 or len(falling) == 0:
        return []

    all_edges = [(r, "R") for r in rising] + [(f, "F") for f in falling]
    all_edges.sort(key=lambda x: x[0])

    edges = np.array([e[0] for e in all_edges])
    directions = [e[1] for e in all_edges]
    intervals = np.diff(edges)

    # Find and decode frames
    frames = []
    i = 0
    while i < len(intervals):
        if directions[i] == "R" and intervals[i] > 10000:  # Preamble
            frame_start = i + 2
            if frame_start >= len(edges):
                break

            frame_end = frame_start
            for j in range(frame_start, len(intervals)):
                if intervals[j] > 10000:
                    frame_end = j
                    break
            else:
                frame_end = len(edges) - 1

            if frame_end > frame_start:
                bits = decode_manchester(
                    edges[frame_start : frame_end + 1],
                    directions[frame_start : frame_end + 1],
                )
                if len(bits) >= 64:
                    frames.append(bits[:64])

            i = frame_end + 1
        else:
            i += 1

    return frames


def parse_frame(bits):
    b = [int(bits[i : i + 8], 2) for i in range(0, 64, 8)]

    cmd_names = {0x01: "PAUSE", 0x02: "UP", 0x04: "DOWN"}
    cmd = cmd_names.get(b[1], f"0x{b[1]:02X}")

    blind_id = b[4] & 0x0F
    # Remote ID: upper nibble of b4 + bytes 5-6
    remote_id = ((b[4] & 0xF0) << 12) | (b[5] << 8) | b[6]
    counter = b[3]

    checksum_ok = ((sum(b[1:7]) + 0x2B) & 0xFF) == b[7]

    return {
        "cmd": cmd,
        "blind_id": blind_id,
        "b2": b[2],
        "counter": counter,
        "remote_id": remote_id,
        "checksum_ok": checksum_ok,
        "raw": " ".join(f"{x:02X}" for x in b),
    }


if __name__ == "__main__":
    prev_raw = ""
    repeat_count = 0
    buf = np.array([], dtype=np.uint8)
    chunk_bytes = sr

    def print_packet(p, rpt):
        chk = "✓" if p["checksum_ok"] else "✗"
        rpt_str = f"x{rpt}" if rpt > 1 else "  "
        print(
            f"{p['cmd']:5s} blind={p['blind_id']:2d} b2={p['b2']:02X} ctr={p['counter']:02X} remote={p['remote_id']:05X} | {p['raw']} {chk} {rpt_str}"
        )
        sys.stdout.flush()

    while True:
        data = sys.stdin.buffer.read(chunk_bytes)
        if not data:
            break

        buf = np.append(buf, np.frombuffer(data, dtype=np.uint8))

        while len(buf) >= chunk_bytes:
            arr = buf[:chunk_bytes]
            iq = (arr[0::2].astype(np.float32) - 127.5) + 1j * (
                arr[1::2].astype(np.float32) - 127.5
            )

            for bits in process_chunk(iq):
                p = parse_frame(bits)
                if p["raw"] == prev_raw:
                    repeat_count += 1
                else:
                    repeat_count = 1
                    prev_raw = p["raw"]
                print_packet(p, repeat_count)

            buf = buf[chunk_bytes // 2 :]
