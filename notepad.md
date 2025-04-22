
rotation stuff
x = x * cos - y * sin
y = x * sin + y * cos

fix tile rendering, try doing the thing with making textures that dont fill the entire thing's edges the right colors or something
might've fixed it differently actually, by adding 0.5 to the coords

Anyways, might do entity highlights and stuff, hotbar fix. k byee

add texture rendering to quad renderer (change name?)

fix issue with cursor rendering on the back of first character of new line instead of last character of old line

should I make player position centered on bottom left or center?

Text rendering stuff is fun but idk if it's the best thing to do right now. try make things playable first?

make method for getting player center maybe

Instead of start/stop events, have an entity finalization kinda thing
Should modifications not take effect until somehting else is done?
I'm basically reinventing command buffers??
Either way, the current system kinda sucks!

I feel like everything i do is super inefficient with component access and stuff idk. like what do for functions

