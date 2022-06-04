# Some Rack v2 plugin experiments...

![](https://nextcould.roselove.pink/s/Gc5imSxnMpcZXPD/preview)

### HeadlessPatcher
- Made after some discussion in VCV Rack discord #development channel on the idea of running dynamic patches in headless mode
- Adds a cable after 3 seconds and removes it before module removal on quit
- Run only using the included patch, module id is hardcoded and don't overwrite it!
- Try both headless and GUI mode!

It works by spawning a background thread from the module widget onAdd() method. The module sends messages to the thread through a ring buffer to add a new cable.  
The reason for that overkill method is because engine->addCable() attempts to lock a mutex which is __*never ever*__ available during process() and step() doesn't run in headless mode.  
The scope shows a triangle but there is no cable from the osc to the filter because it bypasses cable widgets in the gui, but the cable is there in the engine! :)