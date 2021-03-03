

This is the code to run a trackball I'm building for myself.<br>

It's currently based around either a [Seeeduino XIAO](https://wiki.seeedstudio.com/Seeeduino-XIAO/) or the very similar [Adafruit QT Py](https://www.adafruit.com/product/4600), and a couple of optical mouse sensor breakout boards. I've built two so far, one using [these ADNS-9800 boards](https://www.tindie.com/products/jkicklighter/adns-9800-optical-laser-sensor/), and one using [these PMW3360 breakout boards](https://www.tindie.com/products/jkicklighter/pmw3360-motion-sensor/).<br>

<img src="pictures/both-finished.jpg"><br>

I started this project because my favorite ergonomic trackball, the Trackman Marble FX, is basically no longer viable. I really like the shape of it, but it's PS/2 only, has a number of issues with tracking and durability, and has become really hard to find/expensive. I have a small collection of them, most of which have broken over the years.<br>

I decided to build my own trackball with a shape that fits my hand similar to the old Trackman Marble FX, and incorporated a feature from the Kensington Slimblade (probably my second favorite, although not for its ergonomics): twisting the ball in the Z axis acts like a scroll-wheel (complete with clicky sounds produced by a piezo speaker).<br>

To be able to sense rotation in all three axes, it needs to use two mouse sensors, positioned at different points around the ball. <br>

I'm quite happy with the current state of the project. I prefer using this trackball to any other I've used at this point, which is saying something. :)

The [hardware](hardware) directory contains the OpenSCAD files for the printed plastics. The [src](src) directory has the code.

# BUILD #

I'm building this with PlatformIO/VSCode. The root of the repository should open directly with VSCode.

I do my work on Mac OS X, so I don't know if there will be any issues using it on Windows or Linux.

