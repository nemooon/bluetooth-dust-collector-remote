#!/usr/bin/env python3
"""Extract the bundled S140 7.3.0 + MBR from a Seeed nRF52 bootloader HEX.

The Seeed package bundles the SoftDevice with its own bootloader. Raytac DFU
must receive only the MBR/SoftDevice region; its factory bootloader is kept.
"""

import argparse
from pathlib import Path


def record(address, kind, data):
    body = bytes((len(data), address >> 8, address & 255, kind)) + data
    return ":" + (body + bytes((-sum(body) & 255,))).hex().upper()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()

    memory = {}
    base = 0
    for line in args.source.read_text().splitlines():
        if not line.startswith(":"):
            raise ValueError("Invalid Intel HEX record")
        data = bytes.fromhex(line[1:])
        if len(data) != data[0] + 5 or sum(data) & 255:
            raise ValueError("Invalid Intel HEX length or checksum")
        count, address, kind = data[0], int.from_bytes(data[1:3], "big"), data[3]
        payload = data[4:4 + count]
        if kind == 2:
            base = int.from_bytes(payload, "big") << 4
        elif kind == 4:
            base = int.from_bytes(payload, "big") << 16
        elif kind == 0:
            for offset, value in enumerate(payload):
                absolute = base + address + offset
                if absolute < 0x27000:
                    memory[absolute] = value
        elif kind not in (1, 3, 5):
            raise ValueError(f"Unsupported Intel HEX record type: {kind}")

    if 0 not in memory or 0x1000 not in memory or max(memory) >= 0x27000:
        raise ValueError("Unexpected MBR/SoftDevice address range")
    if max(memory) < 0x26000:
        raise ValueError("SoftDevice region is unexpectedly short")

    lines = []
    upper = None
    addresses = sorted(memory)
    index = 0
    while index < len(addresses):
        start = addresses[index]
        next_upper = start >> 16
        if next_upper != upper:
            upper = next_upper
            lines.append(record(0, 4, upper.to_bytes(2, "big")))
        chunk = bytearray((memory[start],))
        index += 1
        while (index < len(addresses) and len(chunk) < 16 and
               addresses[index] == start + len(chunk) and
               addresses[index] >> 16 == upper):
            chunk.append(memory[addresses[index]])
            index += 1
        lines.append(record(start & 0xFFFF, 0, chunk))
    lines.append(record(0, 1, b""))
    args.destination.write_text("\n".join(lines) + "\n")
    print(f"Wrote {len(memory)} MBR/SoftDevice bytes to {args.destination}")


if __name__ == "__main__":
    main()
