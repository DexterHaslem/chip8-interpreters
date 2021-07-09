# CHIP-8 interpreters
Here are some CHIP-8 interpreters just for fun.

The meat of the interpreter is in `desktop_win_interpreter/c8.c`

## Desktop - SDL2
![interpreter screenshot](https://media.dexterhaslem.com/c8interpreter_desktop.png)

This is a simple windows program. To use drag a ROM into the window.
Dragging a ROM on will always reset the interpreter state. 
There is a maze rom in this repo and release. Check out the 'net for 
lots more, but be aware input in CHIP-8 consists of 16 keys! -

WIP: keyboard input. kinda hairy!

Currently there is only a vc++ solution but it could in theory 
be ported to linux/mac as there is no windows specific stuff not
handled by SDL2 to my knowledge, but I haven't tried it yet.


## TODO

I was originally planning on trying to port to some smaller 8bit micros (PIC16F, EFM8 etc) 
without realizing they do not have enough memory to run interesting roms and i'd have to 
bake one in. 

I'll look at bigger micros, likely something like the [sipeed longan nano](https://longan.sipeed.com/en/#what-is-longan)
because it's beefy enough and has a screen built right in! It's also got a sd interface for loading roms and fonts.. tempting
