# TODO

pause / unpause game

Improve allocation method for archetype pools

make it AAABBBCCC instead of AAA - BBB - CCC. better cache and stuff

ECS: make entity maker class that is like a command buffer you make and then execute at end of entity creation.
Makes making entities easy and efficient. Can also be used to make large numbers of entities at once efficiently

replace old ECS system with new
    - change element to entity

shared components? do i need? do more research - prolly not


fix text formatting

stuff:
Items need to be stored canonically so there is only one ecs element per unique item type + variations. Item elements aren't just made willy nilly


fix terminal out of sync for a frame cause of size constraint stuff and text


actual (new):
fix scrolling up/down with keys on console
more text features (copying & pasting??)

do new gui


## Big picture
- Move stuff to .cpp files 
    √ Player Controls needs this bad
- Multithreading??
- bring back all the old systems
- make stuff not extremely ugly like it is now
- real world generation, perlin noise and junk?
- make physics
- redo entity chunk position changed thing. use flags or something
- fix mac apps
- Make actual stuff like belts and -inserters-
- clean up code, add documentation
- Add back saving and reloading!
    - Restart from scratch? Definitely need to think stuff over again. 
    - Start simple, with just chunks probably
- Controller support! just for fun probably (just movement would be cool, fun learning experience)
    √ started, kinda works

## little guys
- fix motion blur for dynamic objects (use nvidia article)

- Scrolling text box

- gui texture rendering
    - make/steal (preferable) gui textures -
        - button
        - rectangels
        - other stuff
    - world textures can be duplicated in guiatlas for now. Eventually maybe make different textures for those

- make inventory work again??? or maybe thats not gonna happen
    - be able to look inside chests and junk
- make bulk ecs functions (add/get multiple components at once)
- fix shader hot loading. issue is a copy is made of all shaders before running so the one i modify is the wrong one
    √ temp fix (i dont know what do)
- make tilemap rendering more SOA, do texcoords then positions
    √ kinda, can still probably optimize tho
- fix console log message select with arrow keys regarding skipping messages like command results
- support middle and bottom top vertical alignments for textboxes.
    - move away from using stringbuffers for console
- text shader use geometry shader just like tilemap and stuff

- make new commands
    - make commands for messing with entities and stuff
    - switch hotkeys out for commands
    - other cool debugging ones, partially for fun :| 

- make static text rendering (for text that doesn't change, increase performance)

- If possible make window open on second monitor by default

### things done
√ Move to openGL rendering
√ Just rethink centered rendering and stuff completely. Should position always mean the center of an entity?
√ Fix save folder path in debug
√ Make a SYSTEM system, instead of just using lambdas for everything
√ make a limited time life component for the sand gun sand
√ figure out a way to have more than 32 components
√ fix text rendering
√ organize files somehow (move filesystem to utils)
√ figure out texture units
√ make log stay open for a few seconds after typing a message
√ add back SDF rendering
    - good for big sizes
√ make types for console result messsages, with different colors (command fail, command success, etc)
√ allow moving the console cursor with left and right arrow keys
√ log errors to in game console as well as regular one
√ bring back rendering chunk borders

----------

## Bugs:
- player shadow is sometimes hidden by other transparent object, depending on location and zoom
- Entities on chunk borders can be rendered multiple times
- setComponent with garbage as the value gives garbage output
- Destroying all entities leaves entity existence information and component information in a broken state (seg faults!!!!)

### top 10 disney actors you didn't know were bugs
√ console cursor doesn't move over for white space, so it's at the wrong spot
√ entities dont get rendered when at edges of screen
√ world isn't rendered until window is resized for some reason

# Things that need to be tested:
- sizeOffsetScale in text rendering

# Current task(s):
Do easy things, clean up code, stop adding more features!!! focus on making the project able to be displayed and understood by humans