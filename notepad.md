Possible use cases for Trigger system

Entity exit:
Destructors, but there are alternatives

Entity enter:
chunkmap
Tracking entities information like for a debug view or something

both: an inserter or something wants to know if an entity in front of it has an inventory component
instead of checking every frame, only check when it gains or loses it

regardless, this current system is pretty much only usable for destructors for which there are already good alternatives
