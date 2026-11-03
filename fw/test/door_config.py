#!/usr/bin/env python3
"""Door configuration tool (USB serial).

Reads the door configuration, asks whether to change it, asks for new
value(s), confirms and writes them. Firmware commands: CFG?, NAME=<name>,
BRPER=<seconds> (0 = no periodic broadcast).
"""

import argparse
import re
import sys
import time

import serial

NAME_MAX = 16
NAME_RE = re.compile(r"^[A-Za-z0-9_-]{1,%d}$" % NAME_MAX)
BRPER_MIN = 10
BRPER_MAX = 24 * 60 * 60

CFG_RE = re.compile(r"^CFG: NAME=(\S+) BRPER=(\d+) CNT=(\d+)")


class LineReader:
    """Serial line reader keeping unprocessed bytes between calls."""

    def __init__(self, port):
        self.port = port
        self.buf = b""

    def reset(self):
        self.port.reset_input_buffer()
        self.buf = b""

    def cfg_line(self, timeout, match=None):
        """Return the first 'CFG: ...' line (matching prefix or ERR) or None on timeout."""
        end = time.monotonic() + timeout
        while True:
            while b"\n" in self.buf:
                raw, self.buf = self.buf.split(b"\n", 1)
                line = raw.decode(errors="replace").strip()
                if not line.startswith("CFG: "):
                    continue  # firmware debug output
                if match is None or line.startswith(match) or line.startswith("CFG: ERR"):
                    return line
            if time.monotonic() >= end:
                return None
            self.buf += self.port.read(self.port.in_waiting or 1)


def send(port, text):
    port.write(text.encode())
    port.flush()


def read_config(rd, timeout):
    rd.reset()
    send(rd.port, "CFG?\n")
    line = rd.cfg_line(timeout, "CFG: NAME=")
    if line is None:
        return None
    m = CFG_RE.match(line)
    if not m:
        return None
    return {"name": m.group(1), "brper": int(m.group(2)), "cnt": int(m.group(3))}


def fmt_brper(s):
    if s == 0:
        return "0 (no periodic broadcast)"
    return f"{s} s ({s / 60:g} min)"


def print_config(cfg):
    print(f"  name:  {cfg['name']}")
    print(f"  brper: {fmt_brper(cfg['brper'])}")
    print(f"  saved: {cfg['cnt']}x")


def ask_yes(prompt, default=False):
    hint = "[Y/n]" if default else "[y/N]"
    ans = input(f"{prompt} {hint} ").strip().lower()
    if not ans:
        return default
    return ans in ("y", "yes")


def ask_name(current):
    while True:
        ans = input(f"Name [{current}]: ").strip()
        if not ans:
            return current
        if NAME_RE.match(ans):
            return ans
        print(f"  invalid: 1-{NAME_MAX} chars, A-Z a-z 0-9 _ -")


def ask_brper(current):
    while True:
        ans = input(f"Broadcast period in s, 0 = off [{current}]: ").strip()
        if not ans:
            return current
        if ans.isdigit():
            v = int(ans)
            if v == 0 or BRPER_MIN <= v <= BRPER_MAX:
                return v
        print(f"  invalid: 0 or {BRPER_MIN}..{BRPER_MAX}")


def main():
    parser = argparse.ArgumentParser(description="Door configuration tool (USB serial).")
    parser.add_argument("--port", default="/dev/ttyACM0", help="USB serial port (default: %(default)s)")
    parser.add_argument("--timeout", type=float, default=2.0, help="answer timeout in seconds (default: %(default)s)")
    parser.add_argument("--save-timeout", type=float, default=30.0,
                        help="wait for flash save, firmware saves when motor is stopped (default: %(default)s)")
    args = parser.parse_args()

    try:
        port = serial.Serial(args.port, baudrate=115200, timeout=0.1)
    except serial.SerialException as e:
        sys.exit(f"Can't open {args.port}: {e}")

    with port:
        rd = LineReader(port)
        cfg = read_config(rd, args.timeout)
        if cfg is None:
            sys.exit("No answer to CFG? (wrong port or firmware without config support?)")

        print(f"Current config ({args.port}):")
        print_config(cfg)

        if not ask_yes("Change config?"):
            return

        name = ask_name(cfg["name"])
        brper = ask_brper(cfg["brper"])

        cmds = []
        print("Changes:")
        if name != cfg["name"]:
            print(f"  name:  {cfg['name']} -> {name}")
            cmds.append((f"NAME={name}", "CFG: NAME="))
        if brper != cfg["brper"]:
            print(f"  brper: {fmt_brper(cfg['brper'])} -> {fmt_brper(brper)}")
            cmds.append((f"BRPER={brper}", "CFG: BRPER="))
        if not cmds:
            print("  none")
            return

        if not ask_yes("Write?"):
            print("Cancelled.")
            return

        # all commands in one message -> one flash save
        rd.reset()
        send(port, "".join(c + "\n" for c, _ in cmds))
        for cmd, expect in cmds:
            line = rd.cfg_line(args.timeout, expect)
            if line is None:
                sys.exit(f"No answer to {cmd}")
            print(f"  {line}")
            if line.startswith("CFG: ERR"):
                sys.exit("Firmware refused the value.")

        print("Waiting for flash save (motor must be stopped) ...")
        line = rd.cfg_line(args.save_timeout, "CFG: save")
        if line is None:
            print("  no save confirmation (motor running?), config is applied but may not be stored yet")
        else:
            print(f"  {line}")

        new = read_config(rd, args.timeout)
        if new is None:
            sys.exit("No answer to CFG? after write")
        print("New config:")
        print_config(new)
        if new["name"] != name or new["brper"] != brper:
            sys.exit("Verification FAILED")
        print("OK")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
        sys.exit(1)
