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

//full_hollow();

module full_hollow()
{
    difference()
    {
        // This replicates most of what's in full()
        body();
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
                    cylinder(h = 50, r = 5.5, $fn=32);
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
                    sphere(r=5, $fn=50);
                }

                // Fix up a spot next to the main button that's got a small unsupported overhang
                button_transform(button_params[0])
                translate([16.5, 0.5, 1])
                sphere(d = 25);
            }
        }
        
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
}

module body_shell_cut()
{
    // This started out as simply:
//    minkowski_difference() {
//        body_unclipped(false, false, $fa=10);
//        sphere(r=3, $fn=16);
//    }

    // This modifies that by replicating parts of body_unclipped, 
    // changing the fillets on the left cut and moving it down and to the right
    // to leave enough thickness to support buttons 1 and 3

    minkowski_difference() 
    {
        difference()
        {
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
        sphere(r=3, $fn=16);
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



