# Motion-Activated Desk Light

ESP32 firmware for a desk light with warm-white and cool-white LEDs. It turns on with a manual switch or when a PIR sensor detects motion, and brightness and colour temperature are adjusted with a rotary encoder.

## Features

- Brightness and colour temperature (warm to cool) control with a KY-040 rotary encoder
- Encoder button toggles which of the two the encoder controls
- Turns on with a manual switch, or for 30 seconds after the PIR sensor detects motion
- Smooth dimming with 5 kHz, 8-bit PWM through MOSFETs
- Over-the-air (OTA) firmware updates over WiFi (hostname `desk-light`)

## Hardware

- Any ESP32 Board
- KY-040 rotary encoder
- AM312 or any viable PIR Sensor
- Warm-white and cool-white COB LED
- 2x IRLZ44N MOSFET (one per LED channel)
- Manual switch

## Pin map

| Function | GPIO |
|---|---|
| Encoder CLK | 14 |
| Encoder DT | 27 |
| Encoder button (SW) | 26 |
| Warm-white LED (MOSFET) | 2 |
| Cool-white LED (MOSFET) | 4 |
| Manual switch | 25 |
| PIR sensor | 15 |

## Setup

1. Install [VS Code](https://code.visualstudio.com/) and the PlatformIO extension.
2. Clone this repo and open the folder in VS Code.
3. In `src/`, copy `secrets.h.example` to `secrets.h` and fill in your own values (WiFi details and the OTA password). `secrets.h` is ignored by git and must never be committed.
4. Build and upload with PlatformIO. Flash over USB the first time. After that, OTA uploads work while the board is on your network.

## Usage

- **Turn the encoder:** adjusts brightness (default mode) or colour temperature
- **Press the encoder:** switches between brightness mode and colour temperature mode
- **Manual switch on:** light stays on
- **Motion detected:** light turns on and stays on for 30 seconds after the last motion

Serial output at 9600 baud shows the current brightness, colour temperature, mode, switch state, and PIR state.

## Known behaviours

- For the first 30 seconds after power-up, the light counts as "motion detected" and turns on, because the motion timer starts at zero. The PIR sensor also needs a warm-up period.
- OTA only starts if WiFi connects within 10 seconds at boot. If the router is slow or down, OTA stays off until the board is reset.

## License

MIT, see [LICENSE](LICENSE).
