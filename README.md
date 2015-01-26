# README #

This is the code to run a trackball I'm building for myself.  The main hardware it's built around is a Sparkfun Pro Micro:

https://www.sparkfun.com/products/12587

and two ADNS-9800 laser optical mouse sensors, which I got from here (including breakout boards and lenses):

https://www.tindie.com/products/jkicklighter/adns-9800-optical-laser-sensor/

original kickstarter link for the breakout boards is here:

https://www.kickstarter.com/projects/1034145369/high-speed-laser-optical-sensor

This code is derived from the code written by the fellow who created the breakout boards.  The firmware files for the sensor, in particular, are taken directly from here:

https://github.com/mrjohnk/ADNS-9800

I've refactored the code substantially, set it up to be able to run two of the sensors (since I'm trying to emulate the "twist-to-scroll" behavior of the Kensington Slimblade, two sensors were necessary), and tweaked the sensitivity, etc. to my taste.

This code may be used for any purpose.  If you post or release something derived from this code, I'd appreciate it if you included a link to this repository and/or mrjohnk's original github repository.