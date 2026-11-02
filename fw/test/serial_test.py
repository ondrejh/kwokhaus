#!/usr/bin/env python3

import argparse
import select
import sys
import termios
import time
import tty
from datetime import datetime

import serial


def log_message(direction, message):
    timestamp = datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S")
    print(f"{timestamp} {direction}: {message}", flush=True)


def main():
    parser = argparse.ArgumentParser(description="Serial test tool for KWOK firmware.")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="serial port (default: %(default)s)")
    parser.add_argument("--timeout", type=float, default=0.2, help="RX idle timeout in seconds (default: %(default)s)")
    parser.add_argument("--delay", type=float, default=3.0, help="wait after last key before sending collected commands in one message (default: %(default)s)")
    args = parser.parse_args()

    if args.timeout <= 0:
        parser.error("--timeout must be greater than zero")
    if args.delay < 0:
        parser.error("--delay must not be negative")
    if not sys.stdin.isatty():
        parser.error("run this tool from an interactive terminal")

    stdin_fd = sys.stdin.fileno()
    terminal_settings = termios.tcgetattr(stdin_fd)

    try:
        with serial.Serial(
            args.port,
            baudrate=9600,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0,
        ) as port:
            tty.setcbreak(stdin_fd)
            print(f"Connected to {args.port} at 9600 baud. O=open, C=close, L=light on, F=light off, ?=status, Q=quit.")
            print(f"Keys pressed within {args.delay:g}s are sent together in one message.")
            actions = {
                "o": "OPEN",
                "c": "CLOSE",
                "l": "LON",
                "f": "LOFF",
                "?": "?",
            }
            rx_buffer = bytearray()
            last_rx_time = None
            pending = []  # commands waiting to be sent together
            last_key_time = None

            while True:
                now = time.monotonic()
                deadlines = []
                if last_rx_time is not None:
                    deadlines.append(last_rx_time + args.timeout)
                if last_key_time is not None:
                    deadlines.append(last_key_time + args.delay)
                wait_timeout = max(0, min(deadlines) - now) if deadlines else None

                readable, _, _ = select.select([port, sys.stdin], [], [], wait_timeout)

                if port in readable:
                    data = port.read(port.in_waiting or 1)
                    if data:
                        rx_buffer.extend(data)
                        last_rx_time = time.monotonic()

                if sys.stdin in readable:
                    key = sys.stdin.read(1).lower()
                    if key == "q":
                        break
                    if key in actions:
                        action = actions[key]
                        if action not in pending:
                            pending.append(action)
                        last_key_time = time.monotonic()
                        print(f"  queued: {' '.join(pending)}", flush=True)

                if last_key_time is not None and time.monotonic() - last_key_time >= args.delay:
                    msg = f"TST: KWOK {' '.join(pending)}"
                    port.write(msg.encode("ascii"))
                    log_message("TX", msg)
                    pending.clear()
                    last_key_time = None

                if last_rx_time is not None and time.monotonic() - last_rx_time >= args.timeout:
                    message = rx_buffer.decode("utf-8", errors="replace").strip()
                    if message:
                        log_message("RX", message)
                    rx_buffer.clear()
                    last_rx_time = None
    except serial.SerialException as error:
        print(f"Serial error: {error}", file=sys.stderr)
        return 1
    finally:
        termios.tcsetattr(stdin_fd, termios.TCSADRAIN, terminal_settings)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())