# Big Pig Bash

An action-packed Arduboy adventure directed by **Hunter Esposito-Flaugher**.

The pigpen is under attack! Guide a tiny but mighty pig through eight quick
rounds, collect ridiculous powers, and defeat the Crow King and Mudzilla.
Big Pig Bash is designed to be welcoming for young players: it has generous
health, forgiving collisions, short rounds, and unlimited instant retries.

## Controls

| Control | Action |
| --- | --- |
| D-pad | Move around the pigpen |
| A (hold or mash) | Auto-aim and fire acorns |
| B | BIG BASH when the top-right meter is full |
| Hold Up + Down | Pause |

Choose a power after each cleared pen with Left/Right and A. The four powers
make acorns faster, fire three acorns, refill health, or recharge BIG BASH.

Boss introductions are short automatic cartoons. Button mashing cannot skip
them, and no button is needed to start the fight.

## Build

The reproducible build uses [PlatformIO](https://platformio.org/) and the
`Arduboy2` library. From this directory:

```sh
pio run
```

The compiled file is `.pio/build/arduboy/firmware.hex`.

Prebuilt versions are available from
[GitHub Releases](https://github.com/bradflaugher/big-pig-bash/releases). Each
release includes a ready-to-play `BigPigBash.hex` file.

## Play Without Hardware

The easiest option is the excellent
[Ardens web player](https://tiberiusbrown.github.io/Ardens/player.html). Download
`BigPigBash.hex` from the latest GitHub Release, open Ardens, and drag the HEX
onto the page. You can also use a locally built `.pio/build/arduboy/firmware.hex`.
Use the arrow keys to move, `A` or `Z` for the Arduboy A button, and `S`, `X`,
or `B` for its B button. Sound works in the browser.

For profiling, GIF recording, memory inspection, and time-travel debugging,
download the desktop version from the
[Ardens releases](https://github.com/tiberiusbrown/Ardens/releases). Open the
HEX through its interface or launch it from a terminal:

```sh
Ardens file=.pio/build/arduboy/firmware.hex
```

Ardens can also perform a headless emulator smoke test. This runs five seconds
of emulated hardware and reports AVR errors without opening a window:

```sh
Ardens headless=5000 file=.pio/build/arduboy/firmware.hex
```

## Automated Checks

`.github/workflows/build.yml` runs on every push and pull request. It installs
the pinned toolchain, compiles against the real ATmega32U4 flash and RAM limits,
checks the diff, and publishes `BigPigBash.hex` as a downloadable GitHub Actions
artifact. A `v*` tag also creates a permanent GitHub Release containing the HEX
and Ardens instructions. The workflow can be run manually from the Actions tab.

## Upload

To upload directly to a connected Arduboy or Arduboy FX:

```sh
pio run --target upload
```

If automatic reset does not catch the device, press its reset button when the
upload command starts. The standard Arduboy system sound preference is
respected (hold B while booting, then press Up for sound or Down for mute).

## Arduboy FX

The game deliberately keeps its executable, artwork, and music in the normal
program image. It therefore needs no separate FX data file and works both on a
classic Arduboy and when launched from an Arduboy FX flash cart.

To try one game without changing the FX menu, upload `firmware.hex` directly.
To keep it in the FX menu, add that HEX file to a flash-cart image with the
[Arduboy Python Utilities](https://github.com/MrBlinky/Arduboy-Python-Utilities)
or a compatible Arduboy cart builder, then write the rebuilt cart image. Back
up the current flash cart before replacing it. Holding Up + Down for roughly
three seconds returns an FX to its loader menu.

## Design Notes

- Authored plus-mask pixel art with independent character animation
- Reproducible sprite generation through `tools/generate_sprites.py`
- Fixed-size actor pools and no dynamic allocation
- 30 FPS action tuned for the ATmega32U4
- Precise asynchronous music and effects through `ArduboyTones`
- Auto-aim, one-hit critters, long damage recovery, and unlimited retries
- Best progress and victory crowns saved in EEPROM
- No game-over punishment: a loss restarts the same round at full health

## Credits

- Game direction and original idea: Hunter Esposito-Flaugher
- Code, artwork, and music are original to this repository

Licensed under the MIT License. See `LICENSE`.
