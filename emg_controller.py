"""
EMG Game Controller - 3 FORCE LEVEL GESTURES
Final Year Project: Real-time Video Game Control via EMG

GESTURES:
  Relax              →  REST   →  nothing
  Soft squeeze       →  LEFT   →  Left Arrow
  Medium squeeze     →  RIGHT  →  Right Arrow
  Hard squeeze       →  JUMP   →  Up Arrow

HOW TO PRACTICE:
  Watch the FORCE BAR on the dashboard.
  The bar fills up as you squeeze harder.
  Zone 1 (low bar)    = LEFT
  Zone 2 (medium bar) = RIGHT
  Zone 3 (full bar)   = JUMP

SETUP:
  1. Upload new EMG_GameController.ino to Arduino
  2. Close Arduino IDE completely
  3. Run this script as Administrator
  4. Open poki.com/en/g/temple-run-2
  5. Click the game window and play!
"""

import serial
import serial.tools.list_ports
import time
import keyboard
import os
from collections import deque

# ================================================================
# SETTINGS
# ================================================================
COM_PORT         = 'COM3'
BAUD_RATE        = 115200
KEY_HOLD         = 0.12      # How long each key is held (seconds)
SERIAL_TIMEOUT   = 2.0

# ================================================================
# KEY MAP — 3 force levels
# ================================================================
KEY_MAP = {
    "0": {"key": None,   "label": "REST  ", "color": "YELLOW"},
    "1": {"key": "left", "label": "LEFT  ", "color": "CYAN"},
    "2": {"key": "right","label": "RIGHT ", "color": "GREEN"},
    "3": {"key": "up",   "label": "JUMP! ", "color": "RED"},
}

# Terminal colors
GREEN  = "\033[92m"
YELLOW = "\033[93m"
CYAN   = "\033[96m"
RED    = "\033[91m"
BLUE   = "\033[94m"
RESET  = "\033[0m"
BOLD   = "\033[1m"

# State
current_key_held  = None
key_hold_start    = 0.0
last_command_time = {}
command_history   = deque(maxlen=20)
session_counts    = {"0": 0, "1": 0, "2": 0, "3": 0}
calibration_info  = {}
current_label     = "REST  "
current_command   = "0"


def connect_serial(port, baud):
    while True:
        try:
            ser = serial.Serial(port, baud, timeout=SERIAL_TIMEOUT)
            time.sleep(2.0)
            ser.reset_input_buffer()
            print(f"{GREEN}Connected to {port}{RESET}")
            return ser
        except serial.SerialException as e:
            print(f"{RED}Cannot connect to {port}: {e}{RESET}")
            ports = [p.device for p in serial.tools.list_ports.comports()]
            print(f"Available ports: {ports}")
            print("Retrying in 3 seconds...")
            time.sleep(3)


def handle_key(command):
    """Press the key for this command."""
    global current_key_held, key_hold_start

    # Release held key if time is up
    if current_key_held is not None:
        if time.time() - key_hold_start >= KEY_HOLD:
            keyboard.release(current_key_held)
            current_key_held = None

    if command not in KEY_MAP:
        return

    key = KEY_MAP[command]["key"]

    if command == "0":
        # Release any held key on rest
        if current_key_held:
            keyboard.release(current_key_held)
            current_key_held = None
        return

    if key is not None:
        last_time = last_command_time.get(command, 0)
        if time.time() - last_time >= 0.05:
            if current_key_held and current_key_held != key:
                keyboard.release(current_key_held)
            keyboard.press(key)
            current_key_held = key
            key_hold_start = time.time()
            last_command_time[command] = time.time()


def release_all():
    global current_key_held
    for entry in KEY_MAP.values():
        if entry["key"]:
            try:
                keyboard.release(entry["key"])
            except:
                pass
    current_key_held = None


def get_force_bar(command):
    """Returns a visual force bar based on current command level."""
    if command == "0":
        return f"{YELLOW}░░░░░░░░░░░░░░░░░░░░{RESET}"
    elif command == "1":
        return f"{CYAN}██████░░░░░░░░░░░░░░{RESET}"
    elif command == "2":
        return f"{GREEN}█████████████░░░░░░░{RESET}"
    elif command == "3":
        return f"{RED}████████████████████{RESET}"
    return f"{YELLOW}░░░░░░░░░░░░░░░░░░░░{RESET}"


def print_dashboard():
    os.system('cls' if os.name == 'nt' else 'clear')

    print(f"{BOLD}{CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━{RESET}")
    print(f"{BOLD}{CYAN}   EMG Controller  |  3 FORCE LEVELS{RESET}")
    print(f"{BOLD}{CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━{RESET}")

    if calibration_info:
        low  = calibration_info.get('T_LOW',  '?')
        mid  = calibration_info.get('T_MID',  '?')
        high = calibration_info.get('T_HIGH', '?')
        print(f"  Zones → {CYAN}Soft:{low}{RESET}  {GREEN}Mid:{mid}{RESET}  {RED}Hard:{high}{RESET}")

    # Force bar
    bar = get_force_bar(current_command)
    label = KEY_MAP.get(current_command, {}).get("label", "REST  ")

    print(f"\n  Gesture:  {BOLD}{label}{RESET}")
    print(f"  Force:    {bar}")

    # Zone guide
    print(f"\n  {YELLOW}░░░░░░{RESET} REST    — relax completely")
    print(f"  {CYAN}██████{RESET} LEFT    — gentle squeeze")
    print(f"  {GREEN}█████████████{RESET} RIGHT   — medium squeeze")
    print(f"  {RED}████████████████████{RESET} JUMP    — hard squeeze")

    # Counts
    print(f"\n  ┌─────────────────────────────────┐")
    print(f"  │  REST  : {session_counts['0']:>5} times             │")
    print(f"  │  LEFT  : {session_counts['1']:>5} times  (soft)     │")
    print(f"  │  RIGHT : {session_counts['2']:>5} times  (medium)   │")
    print(f"  │  JUMP  : {session_counts['3']:>5} times  (hard)     │")
    print(f"  └─────────────────────────────────┘")

    # Recent history
    print(f"\n  Recent: ", end="")
    for cmd in list(command_history)[-8:]:
        if cmd == "1":   print(f"{CYAN}LEFT {RESET} ", end="")
        elif cmd == "2": print(f"{GREEN}RGHT {RESET} ", end="")
        elif cmd == "3": print(f"{RED}JUMP {RESET} ", end="")
        else:            print(f"{YELLOW}REST {RESET} ", end="")
    print()

    print(f"\n  {YELLOW}Ctrl+C to quit{RESET}")
    print(f"{BOLD}{CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━{RESET}")


def parse_calibration(line):
    try:
        parts = line.split(",")
        for part in parts:
            k, v = part.split(":")
            calibration_info[k.strip()] = v.strip()
    except:
        pass


def main():
    global current_label, current_command

    print(f"\n{BOLD}EMG Controller — 3 Force Level Gestures{RESET}")
    print(f"  Soft = LEFT   Medium = RIGHT   Hard = JUMP\n")

    ser = connect_serial(COM_PORT, BAUD_RATE)
    print("Calibrating... RELAX your hand completely for 3 seconds.\n")

    last_print_time = 0

    try:
        while True:

            # Release held key if time is up
            global current_key_held, key_hold_start
            if current_key_held is not None:
                if time.time() - key_hold_start >= KEY_HOLD:
                    keyboard.release(current_key_held)
                    current_key_held = None

            # Read serial
            if ser.in_waiting > 0:
                try:
                    raw = ser.readline().decode('utf-8', errors='ignore').strip()
                except Exception:
                    raw = ""

                if not raw:
                    continue

                # Calibration messages
                if raw.startswith("CAL_START"):
                    print(f"{YELLOW}Calibrating — keep relaxed...{RESET}")
                    continue
                if raw.startswith("CAL_DONE"):
                    print(f"{GREEN}Done! Open Temple Run and flex!{RESET}")
                    time.sleep(1)
                    continue
                if "T_LOW" in raw:
                    parse_calibration(raw)
                    print(f"{CYAN}Thresholds: {raw}{RESET}")
                    continue
                if raw.startswith("RELAX"):
                    continue

                # Handle gesture commands
                if raw in KEY_MAP:
                    session_counts[raw] += 1
                    command_history.append(raw)
                    current_command = raw
                    handle_key(raw)

            # Refresh dashboard at 10Hz
            now = time.time()
            if now - last_print_time >= 0.1:
                print_dashboard()
                last_print_time = now

    except KeyboardInterrupt:
        print(f"\n{YELLOW}Shutting down...{RESET}")
    except serial.SerialException:
        print(f"\n{RED}Arduino disconnected!{RESET}")
    finally:
        release_all()
        try:
            ser.close()
        except:
            pass
        print(f"{GREEN}All keys released. Bye!{RESET}")


if __name__ == "__main__":
    main()
