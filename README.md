# `ray-bench`

Ray tracing benchmark suite using data from modern ray-traced games!

## Features

- Not much yet.

## Build

- Make sure you have the following dependencies installed. Older versions may work, but were not tested.
  - Visual Studio 2026 with MSVC v143 C++ x64 build tools (or later)
  - Windows 11 SDK 10.0.26100.7705 (or later)
  - vcpkg package manager (make sure to integrate it with `vcpkg integrate install`)
- Clone the repository with `--recursive` or run `git submodule update --init --recursive` after cloning.
- Build the solution in Visual Studio or using `msbuild` from the Developer Power Shell for VS.

## Usage

- Run `Tools/inject` project in Visual Studio or `bin/../ray-bench-inject.exe` from the command line.
  - After quitting the game, press `Ctrl` + `C` or close the terminal to stop the injector.

## License

No license yet.
Contact the author for permission to use or modify this code.
See [credits](CREDITS.md) for a full list of third-party software and their licenses.