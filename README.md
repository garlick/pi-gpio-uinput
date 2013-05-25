pi-gpio-uinput
==============

Game Table Buttons for Raspberry Pi

I modded my coffee table, like so many others before me :-)
This seemed like a useful bit of code to share.  It allows
buttons attached to Pi GPIO inputs to generate uinput keypress
events that advmenu and advmame can read.  For that matter, it's
an example of an approach that could be used in a variety of
Pi user interface situations.  Buttons are wired as follows:

```
             3V3
              |
             10k
              |
   |  o-------o---1K---o GPIO input
|--|             
   |  o---gnd
```

/sys/class/gpio is used to read the inputs (set to interrupt on both edges).

Make builds an executable called _pigc_ (for PI Game Controller).
My pi starts it from /etc/rc.local with the -s option.  It must
run as root to access /sys/class/gpio.

GPIO lines are mapped to key events in the table at the top of gpio.c.

I've included my advmame.rc and advmenu.rc files for in case they
are useful to someone doing the same thing as me.
