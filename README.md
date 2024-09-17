<img src="res/icons/ChillyGB.svg" alt="logo" title="ChillyGB" align="right" height="64px" />

# ChillyGB

ChillyGB is a Game Boy (DMG) and Game Boy Color (CGB) emulator written in C that aims to simplicity and accuracy

You can try it now [here](https://chillygb.arci.me) (Note that the audio in WASM port is a little bugged)

## Screenshots

<a><img src="https://github.com/user-attachments/assets/fc0cefd5-0471-4fe1-8094-4e0eb3a62c69" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/08582981-7ebc-43d6-8062-c1875530edc5" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/396220db-a16d-4cd9-bfab-f00d0c73d651" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/a9933d2c-0f7a-43c9-90c3-0dad71591d75" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/52486c65-411b-47d4-9e90-80a797080feb" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/1d16d7a2-b4e2-4c41-ba9e-7f8b267d3a89" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/597f0adb-b9a2-4703-9ca7-d61644ab40d3" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/5c182dfd-0f7d-4cfb-b299-af0314bbc167" width="24.25%"/></a>

## Building

### Linux

#### Dependency Installation
**Fedora**
```bash
sudo dnf install git cmake make raylib raylib-devel gcc
```
**Arch Linux**
```bash
sudo pacman -S git cmake make gcc raylib
```

#### ChillyGB Build

Clone the ChillyGB repository
```bash
git clone https://github.com/AuroraViola/ChillyGB --recursive && cd ChillyGB
```
Build ChillyGB
```bash
cmake -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build --config Release
```

### Windows

TODO

## Planned features

Here is a list of planned features

* **Cartridge support**
  * MBC1M
  * MBC5 Rumble
  * Pocket camera support
* **Emulation Improvements**
  * Accurate sound behaviours
  * STAT IRQ blocking
  * Halt bug
  * Accurate Stop Instruction
  * DMA Conflicts
  * Online multiplayer support via emulated link cable
* **Graphical and particular features**
  * Improve Game Boy Color support
  * Frame blending
  * Rewind
  * Rom folder
  * Rumble
* **Debugger Improvements**
  * Breakpoints
  * Watchpoints
  * APU and channel state
  * VRAM viewer
  * Memory editor

## Not Planned features

These features will not be implemented (at least in this repository)

* SGB borders

## License

This software is licensed under the [GNU General Public License v3.0](https://github.com/AuroraViola/ChillyGB/blob/main/LICENSE.md)