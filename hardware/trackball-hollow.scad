include <BOSL2/std.scad>
include <trackball.scad>

// This uses BOSL2's minkowski_difference to turn the trackball body into a thin shell

// To render this version, you will need to install BOSL2 from here:
// https://github.com/BelfrySCAD/BOSL2

//sensor_params = [
//    [180, true, sensor_type_adns9800],
//    [45, true, sensor_type_adns9800]
//];

//ball_diameter=57;

//full_hollow(true);

module full_hollow(usb_plug = true)
{
    difference()
    {
        // This replicates most of what's in full()
        union()
        {
            body();
            if (usb_plug)
            {
                // Fill in a bit around the USB port
                translate([0, 0, bottom])
                {
                    hull()
                    {
                        rotate([0, 0, 180 + 45])
                        translate([0, -50, 0])
                        {
                            cuboid([30, 20, 12], anchor = FRONT+BOTTOM,
                                rounding = 2, edges = [FRONT+TOP, FRONT+RIGHT, FRONT+RIGHT+TOP]);
                                
//                            translate([0, 0, 4])
//                            rotate([-90, 0, 0])
//                            cyl(d = 10, h = 10, anchor = BOTTOM,
//                                rounding = 2);
                        }
                    }
                }
            }
        }
        ball_cutout();
        bearing_cutouts();
        sensor_cutouts();
        button_cutouts();
        stud_hole_cutouts();

        // These shouldn't be needed anymore, since most of the shell will be hollow
//        sensor_access_cutouts();
//        wire_cutouts();
        
        // The new hollow shell
        difference()
        {
            // This creates a hollow cut which is basically inset from the outer shell by 3mm everywhere.
            body_shell_cut();
            union()
            {
                // Leave support around the ball
                hull()
                {
                    sphere(r = ball_radius + 3);
                    translate([0, 0, bottom])
                    cylinder(h = ball_radius, d = hole_diameter + 6);
                }
                // stud attachment points for the bottom cover
                for(i = stud_locations)
                {
                    translate([i[0], i[1], bottom])
                    cylinder(h = 50, r = 5.5);
                }
                // something for the sensors to attach to
                sensor_supports();

                // Backing for each of the three support bearings
                for(i = bearing_spacing )
                {
                    rotate(i) 
                    rotate([bearing_angle, 0, 0])
                    translate([0, 0, -(ball_radius + (bearing_diameter/2))]) 
                    scale([1, 1, 0.75])
                    sphere(r=5);
                }

                // Fix up a spot next to the main button that's got a small unsupported overhang
                button_transform(button_params[0])
                translate([16.5, 0.5, 1])
                sphere(d = 25);

            }
        }
        
        if (usb_plug)
        {
            // Front outlet that will fit either a fixed cable or a small USB-C breakout board like this one:
            // https://www.amazon.com/dp/B0CB2VFJ54
            // This appears to be a clone of this SparkFun one:
            // https://www.sparkfun.com/sparkfun-usb-c-breakout.html
            // and similar-looking ones are available from AliExpress:
            // https://www.aliexpress.us/item/3256808121945210.html
            // Insert the board into the rectangular cutout with the PCB side towards the top of the trackball, 
            // and the bottom cover will rest against the USB-C connector and help hold it in place.
            // The sensor can be secured with M3x6mm screws, either cap or button head should fit.
            // The wires can be soldered directly to the breakout board, or if you prefer there should be room
            // to solder a 5 or 6 pin JST-XH connector to it (on the side OPPOSITE the USB-C connector, 
            // so it's facing upwards in the assembled trackball) and make the cable with crimp connectors.
            translate([0, 0, bottom])
            {
                rotate([0, 0, 180 + 45])
                translate([0, -42, 0])
                usb_port_cut();
            }
        }
        else
        {
            // Cable outlet at the front (this was part of wire_cutouts, which we've disabled here.)
            translate([0, 0, bottom])
            {
                rotate([90, 0, 230])
                translate([-22, 0, 35])
                rotate([0, 15, 0])
                linear_extrude(height = 25)
                wire_cutout_profile();
            }
        }
        
        // A little extra clearance for the main board
        translate([0, 0, bottom])
        translate([breadboard_offset[0] + 3, breadboard_offset[1], 0])
        hull()
        {
            ccube(breadboard_size[0], breadboard_size[1], 4);
            
            cylinder(h = 20, d = 2);
        }
        
    }

}

//usb_port_cut();
module usb_port_cut()
{
    board_width = 21.75;
    board_length = 12.75;
    board_height = 4.75;
    hole_size = 3.3;
    hole_offsets = [
        [ (13.29 + hole_size) / 2, -(board_length - hole_size) / 2 + 1],
        [-(13.29 + hole_size) / 2, -(board_length - hole_size) / 2 + 1]
    ];
    
    // If true, create holes that can be used to anchor the breakout with M3 screws.
    // If false, try to make pins out of plastic that will hold the board in place.
    // Screw holes are easier to print, and generally fit better.
    screw_holes = true;
    
    difference()
    {
        union()
        {
            // Cavity just the size of the board
            hull()
            {
                // Add just a touch of clearance in case the board is slightly over size
                cuboid([board_width + 0.25, board_length, board_height], anchor = BOTTOM);
                if (false)
                {
                    // Single peaked roof -- this needs really good overhang printing to succeed
                    translate([0, 0, 5])
                    rotate([-90, 0, 0])
                    cylinder(d = 3, h = board_length / 2, anchor = BOTTOM);
                }
                else
                {
                    // Higher roof that comes to a point by the connector cutout.
                    // This should be substantially easier to print.
                    translate([0, 2, 8.5])
                    sphere(d = 3);
                }
            }
            
            hull()
            {
                // hole out the front just the size of the USB plug,
                cuboid([9, 2 + board_length/2, 3], anchor = BOTTOM+BACK,
                rounding = 1.5, edges = [TOP+LEFT, TOP+RIGHT]);
                // with some extra room, if we're using this for a cable instead
                translate([0, 0, 4])
                rotate([90, 0, 0])
                cylinder(d = 3, h = 2 + board_length/2, anchor = BOTTOM);
                
            }
            
            // Room for a JST-XH connector soldered to the board
            // X dimension could be 17.5 here, but making it board_width fixes a bad overhang.
            translate([0, (board_length / 2 + 1), board_height])
            cuboid([board_width, 5.75, 7], anchor = BOTTOM + BACK);

            if (screw_holes)
            {
                for (offset = hole_offsets)
                {
                    translate(offset)
                    {
                        screw_hole_size = 2.75;
                        // Holes for m3 screws to fit into, to secure the breakout board
                        cyl(d = screw_hole_size, h = board_height + 6, anchor = BOT,
                        chamfer2 = screw_hole_size / 2);
                    }
                }
            }

        }
        union()
        {
            if (!screw_holes)
            {
                for (offset = hole_offsets)
                {
                    translate(offset)
                    {
                        // The pins are slightly smaller than the actual hole size, which should make it fit easier.
                        translate([0, 0, board_height - 1])
                        cylinder(d = 3, h = 5, anchor = BOT);
                        // Built-in support for the pin
                        cylinder(d1 = 5, d2 = 3, h = board_height - 1.1, anchor = BOT);
                    }
                }
            }
        }
    }
}


module body_shell_cut()
{
    // This started out as simply:
//    minkowski_difference() {
//        body_unclipped(false, false);
//        sphere(r=3);
//    }

    // This modifies that by replicating parts of body_unclipped, 
    // changing the fillets on the left cut and moving it down and to the right
    // to leave enough thickness to support buttons 1 and 3

    minkowski_difference() 
    {
        difference()
        {
            // The $fa here seriously reduces the complexity of the minkowski
            union($fa=10)
            {
                intersection()
                {
                    // left cut
                    translate([1.5, 0, -2]) // body_shell_cut tweak
                    body_left_cut(15, 25);
                            
                    // right cut
                    body_right_cut();
                }        
            }

            // Trim the front of the body so it's not pointy for no reason
            rotate([0, 0, 45])
            translate([0, 100, -100 + bottom])
            translate([0, -6, 0]) // body_shell_cut tweak: a bit more support under the front edge
            ccube(100, 100, 200);
        }

        // Keeping this shape as simple as possible also helps minkowski complexity
//        sphere(r=3, $fn=16);
        spheroid(r=3, style = "octa", $fn=12);
//        cuboid(3);
    }    
}

// This adds back some material around the sensor cutouts, since the hollow shell 
// would basically leave them floating in space.
module sensor_supports()
{
    for(i = sensor_params )
    {
        rotate(i[0])
        sensor_support(i);
    }
}

module sensor_support(params)
{
    rotate([sensor_angle, 0, 0])
    translate([0, 0, -(lens_thickness + ball_radius + sensor_clearance)]) 
    {
        rotate([sensor_skew_angle, 0, 0])
        {
            if (params[2] == sensor_type_adns9800)
            {
                // difference()
                intersection()
                {
                    cylinder(d=adns9800_board_radius, h=lens_thickness * 2);
                    
                    // translate([-17, 2, 0]) 
                    // cube([adns9800_board_radius + 2, 20, lens_thickness * 2]);

                    translate([0, 0, 10])
                    rotate([0, 90, 0])
                    scale([0.75, 1, 1])
                    rotate([0, 0, 45])
                    cube([25, 25, adns9800_board_radius * 2], center = true);
                }
            }
            else if(params[2] == sensor_type_pmw3360)
            {
                // The board is shifted slightly relative to the lens
                translate([pmw3360_board_offset, 0, 0])
                {
                    translate([0, 0, 10])
                    rotate([0, 90, 0])
                    intersection()
                    {
                        scale([0.75, 1, 1])
                        rotate([0, 0, 45])
                        cube([25, 25, pmw3360_board_width], center = true);

                        translate([0, -5, 0])
                        cube([25, 25, pmw3360_board_width], center = true);
                    }
                }
            }
        }
    }
}

// Seeing these as transparent overlays has been useful while debugging this shape
color([1, 0, 0, 0.5])
union() 
{
//    sphere(r = ball_radius + 10);
//    for(i = stud_locations)
//    {
//        translate([i[0], i[1], bottom])
//        cylinder(h = 50, r = 5);
//    }
//    button_cutouts();
//    wire_cutouts();
//    sensor_cutouts();
//    sensor_access_cutouts();

//    sensor_supports();

      // This primitive is from BOSL2
//      onion(r = ball_radius + 3, ang = 20, orient = DOWN);

//                rotate([0, 0, 45])
//                translate([0, 48, bottom + 6])
//                rotate([45, 0, 0])
//                cube([60, 10, 10], center = true);


//        translate([0, 0, bottom])
//        {
//            rotate([90, 0, 230])
//            translate([-20, 0, 40])
//            linear_extrude(height = 25)
//            wire_cutout_profile();
//        }

//                minkowski_difference() {
//                    body_left_cut($fa=10);
//                    sphere(r=5, $fn=16);
//                }

}



