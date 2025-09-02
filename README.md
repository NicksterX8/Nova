# Nova

An experimental game with a custom game engine written in C++.
This is a passion project entirely created by a single person in their free time.
The goal is primarily to learn new things I've not done before, which is why I have chosen to roll my own GUI system, while most would just use ImGui, for example.

Components:
- A high performance Entity Component System based on archetypes
- Text rendering and formatting
- A basic ECS based GUI
    - A debug console with many commands allowing you to read and change game state while running
- Infinitely expanding procedurally generated chunk based tilemap

Dependencies:
- OpenGL for low level rendering
- glm for vector and matrix math
- SDL3 for the window, input
- SDL3_image for loading textures
- freetype2 for fonts

Uses GoogleTest for unit testing.

## Next tasks
- Create more unique types of entities and mechanics, such as enemies and natural formations
- Improve the GUI, adding more features such as scrolling text boxes, text highlighting, copying, pasting, and more.
- Create more benchmarks of core engine features

## License & Usage
This project is provided for demonstration and portfolio purposes only.
All rights are reserved by the author.

You may view and reference the code, but you may *not* copy, modify, distribute, or use it for any purpose without explicit written permission.
