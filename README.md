# Platformer Game

A simple platformer game built with Godot Engine and C++ extensions.

## Overview

This project is a 2D platformer game featuring a player character navigating levels, collecting items, and reaching win conditions. It uses Godot's GDExtension system for custom C++ modules.

## Features

- Player movement and physics
- Level design with platforms and obstacles
- Main menu and win screen
- Custom C++ classes for game logic

## Prerequisites

- Godot Engine 4.x
- CMake 3.16 or higher
- C++ compiler (GCC/Clang/MSVC)
- Git (for cloning godot-cpp if needed)

## Building

1. Clone the repository and submodules:
   ```
   git clone --recursive https://github.com/your-repo/platformer.git
   cd platformer
   ```

2. Build godot-cpp:
   ```
   cd godot-cpp
   scons platform=linux target=template_release
   cd ..
   ```

3. Build the project:
   ```
   mkdir -p build
   cmake -S . -B build
   cmake --build build
   ```

4. Copy the extension to the Godot project:
   ```
   cp bin/libplatformer.so game/
   ```

## Running

### From Godot Editor
1. Open the Godot project in the `game/new-game-project/` directory.
2. Run the project from the Godot editor.

### From Terminal
If Godot is installed, run the project directly:
```
godot --path /home/lain/platformer/game/new-game-project/
```

### Exported Game
Run the standalone executable:
```
cd game/new-game-project/
./platforming.sh
```

## Project Structure

- `src/`: C++ source files for custom classes
- `game/`: Godot project files
- `godot-cpp/`: Godot C++ bindings
- `build/`: Build artifacts

## Troubleshooting

- Ensure godot-cpp is built for the correct platform and target.
- Check that the `.gdextension` file points to the correct library path.
- For compilation errors, verify CMake and compiler versions.

## License

[Add your license here]