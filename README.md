pi-gpio-uinput
==============

Game Table Buttons for Raspberry Pi

I modded my coffee table, like so many others before me :-)

This little proggie allows buttons attached to Pi GPIO inputs to
generate uinput keypress events that advmenu and advmame can read.

Buttons are wired as follows:

             3V3
              |
             10k
              |
   |  o-------o--1K---o GPIO input
  -|             
   |  o--gnd

/sys/class/gpio is used to read the inputs (set to interrupt on both edges).

Builds an executable called 'pigc' (for pi game controller).
You can start it from your /etc/rc.local if you like.  It must run as root
to access /sys/class/gpio.

See the GPIOnn - key mapping table in gpio.c.
Your advmame.rc and advmenu.rc will need to be set up as well.
I've included mine for reference.
