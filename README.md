<img src="res/icons/ChillyGB.svg" alt="logo" title="ChillyGB" align="right" height="64px" />

# ChillyGB

ChillyGB is an experimental Game Boy (DMG) emulator written in C that aims to simplicity and accuracy

You can try it now [here](https://chillygb.arci.me)

## Screenshots

<a><img src="https://github.com/user-attachments/assets/0b785828-f86e-42ae-841f-d68086bce08f" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/97c05b28-3f57-47eb-a9e1-470bca86d64b" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/396220db-a16d-4cd9-bfab-f00d0c73d651" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/965404b6-5013-4c7d-9c30-a23d2c231f7d" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/dba6679c-6609-4471-bd09-8c26f88ce187" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/552f2f20-bf1d-4359-af1f-8f3f7b8a9f73" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/e81491da-a1f8-4a3d-bff8-0878b83720f3" width="24.25%"/></a>
<a><img src="https://github.com/user-attachments/assets/a69b77fc-d871-4eef-9e77-e508e4f7d7e9" width="24.25%"/></a>

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

#### ChillyGB Installation

Clone the ChillyGB repository
```bash
git clone https://github.com/AuroraViola/ChillyGB && cd ChillyGB
```
Build ChillyGB
```bash
cmake -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build --config Release
```

### Windows

TODO

## Planned features

Here is a list of planned features that need to be implemented for the first stable release

* **Emulation Improvements**
  * Pixel FIFO and more accurate PPU timing
  * Accurate sound behaviours
  * Correct serial emulation
  * STAT IRQ blocking
  * Halt bug
* **Graphical and particular features**
  * Key editor

## Planned features with very low priority

* **Cartridge support**
  * MBC1M
  * MBC5 Rumble
  * Pocket camera support
* **Emulation Improvements**
  * Stop Instruction
  * Accurate VRAM and OAM timing Access
  * DMA Conflicts
  * Online multiplayer support via emulated link cable
* **Graphical and particular features**
  * Screenshots
  * Frame blending
  * Pixel grid
  * Palette editor
  * Rewind
  * Rom folder
* **Debugger Improvements**
  * Breakpoints
  * Watchpoints
  * APU state
  * Stack viewer
  * VRAM viewer
  * Memory editor

## Not Planned features

These features will not be implemented (at least in this repository)

* Gameboy color support
* SGB borders

## License

This software is licensed under the [GNU General Public License v3.0](https://github.com/AuroraViola/ChillyGB/blob/main/LICENSE.md)