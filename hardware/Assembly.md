# Assembly #

This document assumes you are comfortable with 3D printing OpenSCAD models, soldering, and assembling electronics.<br>

Set up the model in `trackball.scad` by setting `ball_diameter` to the diameter of the ball you want to use, and the entries in `sensor_params` to the type of sensor you're using. (You can also tweak the location/number of button cutouts by modifying the contents of `button_params`, although getting a layout that works can take some tweaking. I would advise trying out the default setup first.)

Once you're happy with the setup, render it to an STL file with OpenSCAD, open it in your favorite slicer (I use Cura, but any capable slicer will do), slice it, and print it out. <br>

<img src="../pictures/model-1.png">
<img src="../pictures/model-2.png">

If you're using the microswitch buttons, you'll also need three copies of the model from microswitch-cherry-mx.scad, with appropriate tweaks to `top_extra_overhang`/`top_extra_hinge`. (I'm currently using values of 5/20 for the main button, 5/5 for the right button, and 15/0 for the third button.)<br>
**Update March 12, 2021:** I've refined the third button mount a bit. The mount is now lower-profile than what's in these pictures, and it takes a 20/0 carrier instead of a 15/0. This is called out in `trackball.scad` as `"second thumb button, mk. 2"`.<br>

<img src="../pictures/button-model.png"><br>

I've designed both the trackball model and the microswitch buttons to print with no supports and require only minimal bridging. You will need a printer with decent dimensional accuracy, as there are a couple of places where tolerances are fairly tight.<br>

<img src="../pictures/fresh-print.jpeg"><br>

The 1/8" bearings press-fit into the three holes around the inside of the ball recess. The little bearings are hard to keep hold of, so make sure you're somewhere that you can find them if they escape. (Shag carpet is not great for this. Ask me how I know...) <br>

It takes a bit of pressure to seat them. I recommend doing them one at a time - hold the body so that the bearing hole faces up, place the bearing carefully onto the hole, and then put the ball on top of it and press down on the ball to seat it. <br>

<img src="../pictures/bearings-1.jpeg"><br>

If you need to remove the bearings (maybe to move them to a better version of the body), the model has press-out holes that let you push them out from the back. A stiff wire or ~1.5mm allen wrench should fit right through. You'll have to remove the main button to get the one on the left.<br>

<img src="../pictures/bearings-2.jpeg"><br>
<img src="../pictures/bearings-3.jpeg"><br>

Solder wires to the switches and sensors. I like to use break-apart male header pins on the breadboard end of the wires, as they make nice, low-profile plugs that hold well in the breadboard.<br>

Alternately you could skip the breadboard and solder everything onto a piece of perf-board or perma-proto (if you're the sort of person who does crossword puzzles in pen ;) ).<br>

<img src="../pictures/board-4.jpeg"><br>

For the sensors, I use 6-conductor ribbon cable, and solder the breadboard end of the wires onto a strip of header pins. I arrange the wires so that they match up with the ordering of the ground, power, and SPI pins that are all together on one side of the XIAO, with the Chip Select wire at the end adjacent to the clock pin. For sensor 1 this gets plugged into the breadboard adjacent to the XIAO and uses the D7 pin for chip-select, and for sensor 2 I leave an empty space (actually making it a 7 pin connector with one missing pin), and bring D6 across from the other side of the breadboard to put it on the next row over. 

<img src="https://files.seeedstudio.com/wiki/Seeeduino-XIAO/img/Seeeduino-XIAO-pinout.jpg">
<img src="../pictures/board-6.jpeg"><br>

The wires for the buttons connect to A0/A1/A2 on the opposite side of the chip, and the piezo speaker runs to A3 via breadboard wires. My layout has three rows of grounds on the breadboard just past A0 to make it easy to plug in the other end of the switch harnesses, but you could get away with one or two rows if you crowd them a bit.<br>

This leaves A4/A5 (the i2c pins) open if you want to [hook up a display and see the images from the sensors](https://youtu.be/j6Hdsi4Or-g) (see the `SENSOR_DISPLAY` define in trackball.cpp for more info on this).<br>

<img src="../pictures/board-7.jpeg"><br>

Here are a few more shots of the board layout:<br>

<img src="../pictures/board-1.jpeg"><br>
<img src="../pictures/board-2.jpeg"><br>
<img src="../pictures/board-3.jpeg"><br>
<img src="../pictures/board-5.jpeg"><br>

The sensors mount in the cutouts designed for them with 2mm machine screws. The length of the screws isn't critical, but they need to be short enough that they don't stick out and contact the ball.<br>

There are access holes for a screwdriver in-line with each screw hole, and cutouts that allow you to insert the sensor from the bottom of the body. It can be a bit of a dexterity challenge to get all the pieces lined up (especially with the loose lens on the pnw3360), but once the screws catch it should come together.<br>

<img src="../pictures/sensor-1.jpeg"><br>
<img src="../pictures/sensor-2.jpeg"><br>


The microswitch buttons are set up so that you insert the microswitch from the bottom and slide it sideways to approximately the middle of the switch. Check the orientation of the switch and make sure the switch part is under the bit that sticks down from the top -- if it's the other way around it won't do much. <br>

Once the switch is installed it should have a satisfying mouse-like "click" when you press the top down.  Make sure you click it a few dozen times before continuing. (Yes, this is a mandatory part of the assembly process.) <br>

<img src="../pictures/button.jpg"><br>
<img src="../pictures/button-1.jpeg"><br>
<img src="../pictures/button-2.jpeg"><br>
<img src="../pictures/button-3.jpeg"><br>

The button assembies press-fit into the square holes in the body.<br> 

<img src="../pictures/button-4.jpeg"><br>
<img src="../pictures/button-5.jpeg"><br>

The wires run through the wiring channels underneath.<br>

Carefully bundle the extra wires above the breadboard, plug in your USB cable, and insert the breadboard into the matching cutout under the hand-rest. Route the wires around the sensor, and ensure they don't get pinched between the sides of the breadboard and the body.<br>

The bottom of the breadboard should sit flush with the bottom of the body, and all the wires should run thorugh the cutouts, so the whole thing should be able to sit flat on the table when you're done.<br>

<img src="../pictures/bottom-1.jpeg"><br>

To finish it all off, print a copy of the `bottom_cover()` module from trackball.scad. It has little round nubs that snap into matching holes in the bottom of the body and hold it in place. It's likely to be a bit hard to snap on and off the first couple of times (dependent on the dimensional accuracy of your printer).<br>

<img src="../pictures/bottom-2.jpeg"><br>
<img src="../pictures/bottom-3.jpeg"><br>
<img src="../pictures/bottom-5.jpeg"><br>

I recommend sticking some very thin rubber feet or adhesive rubber sheet to the bottom, to keep it from sliding around on the desk. I used the leftovers from some little sheets of adhesive rubber feet I had lying around, which are only around 1mm thick. <br>

<img src="../pictures/bottom-4.jpeg"><br>

That's it. Enjoy! <br>

<img src="../pictures/top-1.jpeg"><br>
<img src="../pictures/top-2.jpeg"><br>
<img src="../pictures/top-3.jpeg"><br>
