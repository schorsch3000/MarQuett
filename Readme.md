# MarQueeTT[ino]

## A MAX7221 LED-Matrix Scrolling Marquee controlled via MQTT

### MQTT Topics

#### Global Topics

##### Subscribed by Device

- `ledMatrix/text`  : A UTF-8 coded text, max. 4096 bytes long.
- `ledMatrix/intensity`: 0 = lowest, 15 = highest. Default: 1.
- `ledMatrix/delay`: 1 = fastest, 1000 = slowest scrolling. Default: 25 
-  &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; 0 = no scrolling; < 0: negative value sets page cycle time in ms. Default: 5000 ms
- `ledMatrix/blink`: 0 = no blinking; 1 = fastest, 1000 = slowest blinking. Default: 0
- `ledMatrix/enable`: 0 = display off, 1 = display on. Default: 1

Default values are configurable in `local_config.h`. You can also use retained messages, preferably with individual topics (see below).

**Extension:** text channels, `<cno>` = `0`..`9`

- `ledMatrix/text/<cno>`    — text for a specific channel (`.../text/0` = same as `.../text`)
- `ledMatrix/channel`       — set channels to be displayed
  - `<cno1>[,<cno2>...]`    — list of channels
  - `<cno1>`                — only one channel
  - "" (empty)              — only channel 0


#### Individual Topics

##### Subscribed by Device

Same as global topics, but `ledMatrix/<aabbcc>/...` instead of `ledMatrix/...` (<aabbcc> == serial number in 3 hex bytes).

TBD: priority of individual topics over global topics ?

##### Published by Device

- `ledMatrix/<aabbcc>/status`:
  - `startup`       — sent on system start (after first MQTT connect)
  - `version x.y.z` — sent on system start (after first MQTT connect)
  - `reconnect`     — sent on MQTT reconnect
  - `repeat`        — sent at end of a sequence (TBD: for each channel or total sequence?)
  - `offline`       — sent as will when the MQTT connection disconnects unexpectedly

### Setup

#### Wiring

Connect your display's X pin to your controller's Y pin:

- `Data IN` to `MOSI`
- `CLK` to `SCK`
- `CS` to any digial port except `MISO` or `SS` (e.g. `D4`) 

See https://github.com/bartoszbielawski/LEDMatrixDriver#pin-selection for more information

#### Software

- Copy `local_config.dist.h` to `local_config.h` and fill in WLAN credentials. 
- Check the `LEDMATRIX_`* constants according to your hardware setup.
- You might also want to change the default values.

### Change Ideas

- Use `ledMatrix/all` instead of `ledMatrix` as a prefix for global topics so `ledMatrix/all/#` matches all global topics

### Extension Ideas

- WS2812 LED (one or more?) for quick signalling
- acoustic output via piezo element for signalling
- some push buttons for remote feedback or for local display control (cycle next, acknowledge, etc.)

