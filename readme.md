Orbiter 2 Smart Sensor Firmware

===

This is an **unofficial** firmware for the [Orbiter 2 Smart Sensor](https://www.orbiterprojects.com/smart-sensor-for-orbiter-v2/) based on the firmware found in [LorinczElectronics/Orbiter-2-Smart-Sensor](https://github.com/LorinczElectronics/Orbiter-2-Smart-Sensor).

The sensor uses an ATtiny202 to control the LEDs, send the signals to the toolhead sensor and filter false-sensor triggers.

# Shared flashing and communication pin

The filament sensors FUT (**F**ilament **U**nload/**T**angle) pin is connected to both PA0 and PA2, the latter through a 220 Ohm resistor:
```
PA0 (UPDI) ─────────┬──── FUT
                    │
PA2 ────[220 Ohm]───┘
```

PA0 is used for UPDI programming (flashing firmware), while PA2 is configured as an output during regular operation to indicate filament
unload button presses or tangles.

As an output, PA2 actively drives the signal `HIGH` or `LOW`. With UPDI communication on the same line, this can cause electrical interference.
For example, PA2 may be set to `HIGH` while the UPDI programmer attempts to transmit `LOW`, the conflicting signals can prevent communication,
potentially making the sensor impossible to reflash through its connector.

To prevent this, the firmware provides two recovery modes that configure PA2 as high-impedance (input), disabling its output
driver so the UPDI programmer can control the line.
- **Normal programming mode**: Hold both the unload and tangle buttons for approximately four seconds. This matches the flashing mode present in the original firmware.
- **Boot recovery mode**: Hold the unload button while powering on the sensor.

Both modes are indicated by flashing white LEDs.

# Hard-recovery

By default, `PA2` is configured as a high-impedance input after reset. It only interferes with UPDI when the firmware configures it as an output.
In the rare case that this may happen to you, there is way to recover the sensor by desoldering the 220 Ohm resistor connecting the two lines.

# Toolchain

Before you can build this project, some dependencies have to be installed. Distributions may include an outdated compiler or ship with incomplete
device specifications. If you are having trouble, you may have to install the [latest toolchain](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers).

For flashing you have to install `avrdude`, to install everything you need in ubuntu, you can use
```bash
$ sudo apt-get install make gcc-avr avr-libc avrdude
```

# Build

You can build the project by running
```
make
```

The file for flashing will be in `build/` called `orbiter.hex`.

## Configuration

Firmware features can be configured in [`src/config.h`](src/config.h) or overridden at build time using `CPPFLAGS`.

| Option               | Default            | Available values                                      | Description                                                                       |
| -------------------- | ------------------ | ----------------------------------------------------- | --------------------------------------------------------------------------------- |
| `INSERT_DEBOUNCE_MS` | `5`                | `0`, `5`, `10`, `20`, `100`                           | Filament insertion debounce in milliseconds                                       |
| `REMOVE_DEBOUNCE_MS` | `5`                | `0`, `5`, `10`, `20`, `100`                           | Filament removal debounce in milliseconds                                         |
| `STARTUP_ANIMATION`  | `STARTUP_RAINBOW` | `STARTUP_NONE`, `STARTUP_ORIGINAL`, `STARTUP_RAINBOW` | Selects the RGB startup animation                                                 |
| `UNLOAD_LIMIT`       | `10`               | `10`, `2`                                             | Original unload-button filtering or shorter experimental filtering                |
| `CPU_CLOCK_MHZ`      | `20`               | `16`, `20`                                            | CPU oscillator frequency, which must match the chip's existing fuse configuration |

For example, to build with the rainbow animation and 10 ms filament debounce:

```bash
make CPPFLAGS="-DSTARTUP_ANIMATION=STARTUP_RAINBOW -DINSERT_DEBOUNCE_MS=10 -DREMOVE_DEBOUNCE_MS=10"
```

To build without a startup animation:

```bash
make CPPFLAGS="-DSTARTUP_ANIMATION=STARTUP_NONE"
```

Running `make` performs a clean build, so changing configuration options does not require manually running `make clean`.

**Note:** `CPU_CLOCK_MHZ` does not modify oscillator fuses. Select the frequency matching your microcontroller's existing configuration.

# Flashing

## Download firmware from GitHub Actions

Open the repository's **Actions** tab, select **Firmware**, and open a successful
run for the revision you want. Download and extract the
`orbiter-attiny202-<commit>` artifact from the run's **Artifacts** section.

The HEX targets the sensor's factory-configured 20 MHz ATtiny202 and uses
rainbow startup, 5 ms insertion/removal debounce, and the original unload filter.
Flashing this file does not change fuses.

The hex file has to be flashed to the sensor, unfortunately it is not possible to do this without a programmer.

## Serial to TTL

You can use a serial to TTL adapter like an FT232, CH340, CP2102, ...

You need to connect it like this to the sensor:
```
USB-to-TTL adapter      Orbiter Sensor

TX ───[4.7 kΩ]───┬───── FTU / UPDI
RX ──────────────┘
5V ──────────────────── 5V
GND ─────────────────── GND
```

## Arduino UNO

Another option is to use a 5V microcontroller, like the Arduino UNO with [jtag2updi](https://github.com/ElTangas/jtag2updi) for flashing.

My wiring looked like this:
```
Arduino Uno (jtag2updi)      Orbiter Sensor

D6 ─────[4.7 kΩ]─────────── FTU / UPDI (pin 1)
                            FS         (pin 2) — not connected
5V ──────────────────────── +5V        (pin 3)
GND ─────────────────────── GND        (pin 4)
```

and to suppress the auto-reset, you can connect a resistor between 5V and RESET.

```
RESET ───[100 Ω]─────────── 5V
```
Other values may work as well.

## Flashing mode

Like previously described when the PA2 pin is used as an output pin, you can not flash to the sensor, therefore you need to put it into a special
update mode. The official repository describes this [here](https://github.com/LorinczElectronics/Orbiter-2-Smart-Sensor/tree/main/Firmware).
In case someone is struggling with triggering this:
1. Connect the sensor to 5V and GND of the Arduino.
2. Connect the FUT (FTU) pin with the FS pin
3. Then press the button closer to the LEDs and the large one on the other side for a few seconds.
4. The RGB LED should start blinking in white, indicating that it is in flashing mode.
5. For flashing, remove the bridge between FUT and FS pin, otherwise you may get an invalid mcu state error in avrdude.

You can leave the FUT pin connected to while you do the bridging, it is not necessary to remove it.
After you have flashed this firmware, you can enter the flashing mode, by removing power, then holding the filament unload button
while it powers up.

## Uploading the firmware to the sensor

This assumes you have an ATtiny202, there seem to be some sensors with an ATtiny412, I have not tested the firmware for this.

For jtag2updi, you can flash with
```
avrdude -c jtag2updi -P COM8 -p t202 -A -U flash:w:build/orbiter.hex:i
```
and with a USB-to-TTL using SerialUPDI:
```
avrdude -c serialupdi -P COM8 -p t202 -A -U flash:w:build/orbiter.hex:i
```
Note that your programmer will likely be on a different serial port, therefore you should adjust the `COM8` to that one.

### Fuse configuration

The flashing commands above only write the application flash and **do not modify the microcontroller's fuses**, EEPROM or lock bits. AVRDUDE uses flash-page erasure for UPDI programming, so a full chip erase is not necessary.

In particular, the existing UPDI and oscillator fuse settings are preserved. Make sure `CPU_CLOCK_MHZ` in `src/config.h` matches the microcontroller's oscillator configuration.

Avoid explicitly writing fuses or using the `-e` (chip erase) option unless you understand the consequences.

# LLM Disclaimer

This firmware was developed with substantial assistance from Large Language Models (LLMs), including reverse engineering, implementation and testing. While the code has undergone automated testing and some hardware validation, it may still contain bugs or unexpected behavior.

**Use at your own risk.** The firmware is provided without warranty, and I cannot guarantee that it will work correctly on every hardware revision or prevent the sensor from becoming unprogrammable.

# License

This project is dedicated to the public domain, like the original project.
