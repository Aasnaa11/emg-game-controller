 # EMG-Based Game Control Interface

Control video games using real-time muscle signals —
no controller needed. Built with Arduino and Python,
this system reads EMG (electromyography) signals and
translates physical gestures into keyboard inputs.

Demonstrated at university innovation showcase, 2026.

---

## How It Works

1. EMG sensors attached to forearm detect muscle activity
2. Arduino reads and classifies signal strength in real time
3. Python script maps signal levels to keyboard inputs
4. Player controls the game purely through muscle contractions

---

## Gesture Mapping

| Gesture        | Force Level | Action      |
|----------------|-------------|-------------|
| Relax          | None        | REST        |
| Soft squeeze   | Low         | LEFT arrow  |
| Medium squeeze | Medium      | RIGHT arrow |
| Hard squeeze   | High        | JUMP        |

---

## Features

- Real-time EMG signal acquisition and classification
- 3-zone force detection with auto-calibration on startup
- Live terminal dashboard showing force bar and gesture history
- Automatic key press and release handling
- Session stats tracking per gesture type

---

## Hardware

- Arduino (any model with analog input)
- EMG muscle sensor module
- Forearm electrode pads
- USB connection to PC

---

## Software

- Arduino sketch handles signal reading and calibration
- Python script handles gesture classification and key control

---

## Setup

1. Connect EMG sensor to Arduino analog pin
2. Upload `EMG_GameController.ino` to Arduino
3. Close Arduino IDE completely
4. Install Python dependencies:

```bash
pip install pyserial keyboard
```

5. Set your COM port in `emg_controller.py`:

```python
COM_PORT = 'YOUR_COM_PORT'  # e.g. COM3 on Windows
```

6. Run as Administrator:

```bash
python emg_controller.py
```

7. Relax your hand for 3 seconds during calibration
8. Open your game and play

---

## Tech Stack

C++ · Arduino · Python · PySerial · Keyboard Library
Real-Time Signal Processing · Hardware-Software Interface

---

## Files

| File | Description |
|---|---|
| `EMG_GameController.ino` | Arduino sketch — signal reading and calibration |
| `emg_controller.py` | Python script — gesture classification and key control |

---

## Status

Completed · Demonstrated at university innovation showcase · 2026
