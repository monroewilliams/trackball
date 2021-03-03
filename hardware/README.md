# Printables #

This directory contains the [OpenSCAD](https://www.openscad.org) models for the trackball body and microswitch buttons I've designed.

The trackball body model is in `trackball.scad`. There are many parameters that can be tweaked, see the comments in that file for details. 

It's designed using 14mm square holes for the buttons, which should fit either standard Cherry MX (or compatible) keyboard switches or the microswitch-based button I've designed for the project.

The model for the button is in `microswitch-cherry-mx.scad`. The variables at the top set front and rear overhangs, which can match up with cutouts in the trackball body to act as recessed buttons with minimal travel.

If you're thinking about putting one together yourself, take a look at the [assembly guide](Assembly.md).

# Components #

In addition to the 3D printed parts, you will need the following:

1 - Seeeduino XIAO microcontroller (alhtough any controller that has a binding for the Arduino library in PlatformIO should be easy to adapt):

https://wiki.seeedstudio.com/Seeeduino-XIAO/

I've also swapped in an Adafruit QT Py (it's very similar with the same pinout and processor). It has a couple of niceties over the XIAO, including an actual reset button and an RGB LED that could be used to light things up. <br>

I'm currently having some issues with it not dealing well with switching inputs on my KVM switch (it stops getting HID reports to the host and I don't know why), so be aware of that. <br>

https://www.adafruit.com/product/4600 <br>

2 -  Optical mouse sensors. I originally started this project with ADNS-9800 laser optical mouse sensor breakouts from here:

https://www.tindie.com/products/jkicklighter/adns-9800-optical-laser-sensor/

and have since picked up two PMW3360 optical mouse sensor breakouts from here:

https://www.tindie.com/products/jkicklighter/pmw3360-motion-sensor/

The plastics have options to use either kind (or even one of each), and the code also supports both kinds.<br>

It looks like the PMW3389 breakout from the same source would also fit the plastics. The code doesn't yet support it, but it looks like adding support for it in the code would be pretty trivial.<br>

https://www.tindie.com/products/jkicklighter/pmw3389-motion-sensor/<br>

Either:<br>

1 - regulation 2 1/4" (~57mm) billiard ball. If you can find one with some visual texture, it will make the optical sensors work better. I'm using the cue ball from a pearlescent set I bought from eBay just for this project, but one can probably find individual ones online if you don't want a whole set. <br>

The set I bought: https://www.ozonebilliards.com/product/pro-series-kandy-pearl-ball-set <br>
Similar single cue ball: https://www.paramountindustriesinc.com/products/pearlescent-cue-ball <br>

or:<br>

1 - 55mm ball designed for an optical trackball. These are nice because they've got a sort of "sparkly" visual texture that works really well with the optical sensors. This ball will fit and work fine in a body printed with a ball diameter of 57mm, but I would recommend bumping ball_diameter down to 55 in trackball.scad for a better fit.<br>

The Kensington Slimblade and Kensington Expert Mouse trackballs use this size, as does the Perixx Periboard combination keyboard/trackball. Perixx sells a compatible replacement ball, which is a good way to get one without buying extra hardware you don't need:<br>

https://www.amazon.com/gp/product/B07DXBMT6Z/<br>

1 - USB-C cable with a relatively small connector. There's not a lot of room under the printed shell for the cable plug.<br>

I've modeled a channel for the cable to run out the front of the body, but I haven't yet found a cable that can plug into the microcontroller and make the turn to get into the channel, which is why the cable currently sticks out the side. I may eventually switch to a processor that has solder-pads for the USB connection, or hack together a cable with a solder-on USB-C plug, but for now I'm using one of these:<br>

https://www.amazon.com/gp/product/B081F1VR6W/<br>

1 - Standard mini breadboard (170 tie point version), available through many sources:<br>

https://www.amazon.com/Breadboard-Solderless-Prototype-Different-Raspberry/dp/B07LF71ZTS/<br>

3 - ceramic ball bearings, diameter 1/8 inch (3.175mm), which press-fit into holes in the printed body. 
They're cheap in packs of 25 or 50, and I recommend getting some extras because they're really easy to lose.
I'm using these:<br>

https://www.amazon.com/gp/product/B081SNH8J5/<br>

these should work equally well:<br>

https://www.amazon.com/gp/product/B07ZKPZK8Y/<br>

3 - Subminiature microswitches, OMRON D2FC-F-7N or equivalent:<br>

https://www.amazon.com/D2FC-F-7N-Micro-Switch-Microswitch-Switches/dp/B085LB7FZY/<br>

1 - micro piezo speaker if you want twist-scrolling to make a "click" sound. (If you don't want this, just undefine PIN_PIEZO in the trackball source and skip it). These are available from multiple sources, I've used one of these:<br>

https://www.adafruit.com/product/160<br>

and also one of these (after desoldering the wires):<br>

https://www.amazon.com/gp/product/B01MR1A4NV/<br>

4 - M2 x 6mm machine screws to hold the sensors in place. (I originally used some tiny self-tapping screws I had in my junk parts stash, but have since enlarged the screw holes to fit machine screws -- they should hold fine in PETG as long as you don't over-tighten them.)<br>

misc - Hookup wires, solder, etc. for wiring up the sensors and switches. I used some old ribbon cable for the sensor wiring to keep things tidy. I find that soldering the wires to break-apart male header pins makes for solid plug-in attachement to the breadboard.<br>

Optional: Thin adhesive-backed rubber to make non-skid feet for the bottom of the trackball.  I used some leftover sticky-backed rubber from a sheet of rubber feet, but something like this ought to work well:<br>

https://www.amazon.com/Adhesive-Resistant-Silicone-Material-Protection/dp/B07GL9MHW7/<br>


