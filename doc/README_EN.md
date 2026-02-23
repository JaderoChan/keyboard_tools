# Keyboard Tools

[ English | [简体中文](README_ZH.md) ]

A cross-platform keyboard event listening and sending library written in C++. Originally derived from the `KeyboardHook` module in the [Global Hotkey](https://github.com/JaderoChan/global_hotkey) library.

This library by original C library ([Keyboard Tool](https://github.com/JaderoChan/keyboard_tool)) migrated to the C++, so the source has many **C++-unlike** behavior.

## Features

- Written in C++11
- Supports Windows, Linux, and macOS

## How to Build?

1. Run the `install` script in the root directory to build the library and example programs and last install them to `./install` directory.
2. Use `CMake`, for example:

    ```sh
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config release -j
    ```

## Build Options

- `KEYBOARD_TOOLS_BUILD_SHARED`

    Whether to build a dynamic library. Defaults to `OFF`.

- `KEYBOARD_TOOLS_WITH_KEYUTIL`

    Whether to compile the `Key utility` module, which includes functions for mapping `Key` to `Native key code`, as well as conversion functions between `Key` and strings. If you already have a `Native key code` mapping utility, you can set this option to `OFF`. Defaults to `ON`.

- `KEYBOARD_TOOLS_BUILD_EXAMPLE`

    Whether to build example programs.

## Notes

- Generally, administrator privileges are required for proper execution.
- Do not call the `stop` function within the thread where the `EventHandler` is running.
