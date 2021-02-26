# README #

This directory contains the [OpenSCAD](https://www.openscad.org) models for the trackball body and microswitch buttons I've designed.

The trackball body model is in `trackball.scad`. There are many parameters that can be tweaked, see the comments in that file for details. 

It's designed using 14mm square holes for the buttons, which should fit either standard Cherry MX (or compatible) keyboard switches or the microswitch-based button I've designed for the project.

The model for the button is in `microswitch-cherry-mx.scad`. The variables at the top set front and rear overhangs, which can match up with cutouts in the trackball body to act as recessed buttons with minimal travel.

Other components I'm using in the build:

1 - Seeeduino XIAO microcontroller (alhtough any controller that has a binding for the Arduino library in PlatformIO should be easy to adapt):

https://wiki.seeedstudio.com/Seeeduino-XIAO/

The Adafruit QT Py is very similar (with the same pinout and processor) and should work with minimal changes to the code. It has a couple of niceties over the XIAO, including an actual reset button and an RGB LED that could be used to light things up.

https://www.adafruit.com/product/4600

2 -  Optical mouse sensors. I origially started this project with ADNS-9800 laser optical mouse sensor breakouts from here:

https://www.tindie.com/products/jkicklighter/adns-9800-optical-laser-sensor/

and have since picked up two PMW3360 optical mouse sensor breakouts from here:

https://www.tindie.com/products/jkicklighter/pmw3360-motion-sensor/

The plastics have options to use either kind, or even one of each. I'm working on adding support for the PMW3360 to the code.<br>
It looks like the PMW3389 breakout from the same source would also fit the plastics, although I'd have to add support for it in the code:

https://www.tindie.com/products/jkicklighter/pmw3389-motion-sensor/

Either:

1 - regulation 2 1/4" (~57mm) billiard ball. If you can find one with some visual texture, it will make the optical sensors work better. I'm using the cue ball from a pearlescent set I bought from eBay just for this project, but one can probably find individual ones online if you don't want a whole set.

The set I bought: https://www.ozonebilliards.com/product/pro-series-kandy-pearl-ball-set <br>
Similar single cue ball: https://www.paramountindustriesinc.com/products/pearlescent-cue-ball

or:

1 - 55mm ball designed for an optical trackball. These are nice because they've got a sort of "sparkly" visual texture that works really well with the optical sensors. This ball will fit and work fine in a body printed with a ball diameter of 57mm, but I would recommend bumping ball_diameter down to 55 in trackball.scad for a better fit.<br>
The Kensington Slimblade and Kensington Expert Mouse trackballs use this size, as does the Perixx Periboard combination keyboard/trackball. Perixx sells a compatible replacement ball, which is a good way to get one without buying extra hardware you don't need:

https://www.amazon.com/gp/product/B07DXBMT6Z/

1 - Standard mini breadboard (170 tie point version), available through many sources:

https://www.amazon.com/Breadboard-Solderless-Prototype-Different-Raspberry/dp/B07LF71ZTS/

3 - ceramic ball bearings, diameter 1/8 inch (3.175mm), which press-fit into holes in the printed body. 
They're cheap in packs of 25 or 50, and I recommend getting some extras because they're really easy to lose.
I'm using these:

https://www.amazon.com/gp/product/B081SNH8J5/

these should work equally well:

https://www.amazon.com/gp/product/B07ZKPZK8Y/

2 - Subminiature microswitches, OMRON D2FC-F-7N or equivalent:

https://www.amazon.com/D2FC-F-7N-Micro-Switch-Microswitch-Switches/dp/B085LB7FZY/

1 - micro piezo speaker if you want twist-scrolling to make a "click" sound. (If you don't want this, just undefine PIN_PIEZO in the trackball source and skip it). These are available from multiple sources, the one I'm using is:

https://www.adafruit.com/product/160

4 - M2 x 6mm machine screws to hold the sensors in place. (I originally used some tiny self-tapping screws I had in my junk parts stash, but have since enlarged the screw holes to fit machine screws -- they should hold fine in PETG as long as you don't over-tighten them.)

misc - Hookup wires, solder, etc. for wiring up the sensors and switches. I used some old ribbon cable for the sensor wiring to keep things tidy. I find that soldering the wires to break-apart male header pins makes for solid plug-in attachement to the breadboard.

