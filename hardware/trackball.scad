//translate ([0, 0, -15])
//{
//    cube([60, 60, 30], center = true);
//}

body_height=1;
body_diameter=67;

// Pool ball is 57mm in diameter.
// Ball from the Kensington Slimblade is 55mm.
// Bearings are 1/8" == 3.175mm
ball_diameter=57;
ball_clearance=1;
bearing_diameter=3.175;

bottom_clearance=4;
hole_diameter=32;

bearing_angle=60;
bearing_spacing=[ 
    [0, 0, 0],
    [0, 0, 120],
    [0, 0, -120] 
];

sensor_angle=60;
// Having the sensors directly face-on to the ball doesn't work.
// I suspect this is due to specular reflection washing out the surface details.
sensor_skew_angle=10;

// Sensor params are:
// - location angle (clockwise from top)
// - sensor needs bottom access cutout (boolean)
sensor_params = [
    [180, true],
    [45, true]
];
sensor_clearance=2;
lens_thickness=3.3;
sensor_board_thickness=2;
sensor_board_clearance=0;
        

// Give the ball clearance all the way around in the recess
ball_radius=ball_diameter/2;
recess_radius=ball_radius + ball_clearance;

// Shorthand for the minimum z coordinate
bottom = -(recess_radius + bottom_clearance);

// Button params are:
// 0 - azimuth angle (clockwise from top)
// 1 - elevation angle (downwards from level)
// 2 - distance from ball surface to button center (takes ball radius into account)
// 3 - tilt angle away from ball (additive with elevation angle)
// 4 - tilt angle in the clockwise direction
// 5 - cutout rotation
// 6 - front overhang (should match up with parametrs in microswitch_cherry_mx.scad)
// 7 - rear overhang (same)
button_params = [
    // main button
    [110, 27, 12, -5, 7, -3 - 90, 5, 20],

    // second button
    // first model version:
    // [-60, 30, 13, 24, 0, 90, 5, 20]
    [-60, 33, 13, 30, 0, 0, 0, 0],
    // other attempts:
    // [-60, 38, 12, 30, 0, 0 + 90, 5,10]
    // [-30, 34, 14, 40, 33, -9 + 90, 5, 20],
   
    // third (middle) button
    [-10, 19, 13, 30, 30, 12, 0, 0]
];

module button_transform(params)
{
    rotate([0, 0, params[0]])
    rotate([-params[1], 0, 0])
    translate([0, ball_radius + params[2], 0])
    rotate(-[params[3], 0, 0])
    rotate([0, params[4], 0])
    rotate([0, 0, params[5]]) 
    children();
}

button_pod_diameter = 20;

module ccube(x, y, z)
{
    translate([0, 0, z/2])
    cube(center=true, [x,y, abs(z)]);
}

module rcube(x, y, z, r)
{
    translate([0, 0, z < 0 ? z : 0])
    linear_extrude(height = abs(z))
    rrect(x, y, r);
}

module rrect(x, y, r)
{
    translate([-x/2, -y/2, 0])
    translate([r, r, 0])
    offset(r, $fs = r/2)
    square([x - (r * 2), y - (r * 2)]);
}

module prender(convexity = 1)
{
    // Render only in preview mode.
    // Using unconditional render() calls seems to cause my final render to fail

    if ($preview)
    {
        render(convexity)
        children();
    }
    else
    {
        children();
    }
}

module shadow_hull(extra_height = 0)
{
    // This creates an object which is a hull of the object itself and its shadow on the x/y plane.
    hull()
    {
        // the child object
        children();

        // the projection of the child object
        translate([0, 0, bottom - 1 - extra_height]) 
        linear_extrude(height=1)
        projection()
        children();
    }
}

module ball()
{
    color("red")
    sphere(d=ball_diameter);
}

module ball_cutout()
{
    union()
    {
        // spherical part of the recess
        sphere(d=50, $fa=5, $fs=0.1);
        sphere(d=ball_diameter + (ball_clearance * 2));

        // clearance cut for ball removal
        // The angle here isn't critical, it just needs to not look funny.
        // roughly match the angle of the right cut
        rotate([0, -45 , 40])
        rotate([0, 90, 0])
        cylinder(d=ball_diameter + (ball_clearance * 2), h=ball_diameter);


        // bottom hole
        translate ([0, 0, -100])
        cylinder(h=100, d = hole_diameter);
    }
}

module bearing_cutouts()
{
    union()
    {
        // bearing holes
        for(i = bearing_spacing )
        {
            rotate(i)
            rotate([bearing_angle - 180, 0, 0])
            {
                // Bearing hole consists of a hemisphere and a straight cylinder.
                cylinder (h=ball_radius + (bearing_diameter/2), d=bearing_diameter, $fn=16);
                translate ([0,0,ball_radius + (bearing_diameter/2)])
                    sphere (d=bearing_diameter, $fn=16);

                // through-hole to push out the bearing
                cylinder (h=ball_radius + bearing_diameter + 100, d=2.5, $fn=16);
            }
        }
    }
}

module button_cutout(front_overhang = 0, rear_overhang = 0)
{
    union() 
    {
        translate([0, 0, -10])
        {
            // the hole the switch fits into
            ccube(14, 14, 20);
        }

        if ((front_overhang == 0) && (rear_overhang == 0))
        {
            // 3mm cut around the hole for seating/overhang and keycap
            rcube(20, 20, 40, 1);
        }
        else
        {
            // 1mm cut around the hole for seating/overhang
            ccube(16, 16, 40);
        }

        // Front and rear overhangs are intended to match up with the overhang parameters in 
        // microswitch-cherry-mx.scad.
        if (front_overhang > 0)
        {
            translate([-7.5, 7, 0])
            cube([15, front_overhang + 3, 6]);
        }

        if (rear_overhang > 0)
        {
            translate([-7.5, -7 - (1 + rear_overhang), 0])
            cube([15, 1 + rear_overhang, 6]);
        }
    }
}

module button_access_base()
{
    translate([0, 0, -10])
    {
        ccube(14, 14, 1);
    }
}

module sensor_cutout()
{
    union()
    {
        // origin of the sensor is at the surface of the lens, 
        // with the lens facing towards +z, and the top of the sensor towards +y
    
        // window portion of lens is roughly 11x9mm. This will make a window.
        color("white", 0.25)
        ccube(11, 9, 6);
        
//        // Keep the intersection of the holes from being an overhang.
//        translate([0, 7.5 / 2, 0])
//        color("white", 0.25)
//        ccube(15, 7.5, 6);
        
        // Lens portion of sensor is 22 x 20 mm, centered on the board
        color("gray", 0.5)
        intersection()
        {
            ccube(22,32, -(lens_thickness + 1));
            translate([0, 0, -(lens_thickness + 2)])
            cylinder(d=32, h=lens_thickness + 3, $fa=5);
        }
    
        // the board is a 32mm circle, ~2mm thick.
        // Add some thickness to the back to leave room for components, etc.
        z = sensor_board_thickness + sensor_board_clearance;
        translate([0,0,-(lens_thickness + z)])
        {
            // The board itself
            color("green", 0.5) cylinder(d=32, h=z, $fa = 5);
            
            // Hemisphere for clearance
            difference()
            {
                color("white", 0.25)
                sphere(d=32, $fa = 5);
                
                translate([0,0,1])
                cylinder(d=32, h=17, $fa=5);
            }
        }
        
        // The screws are 27mm apart, on the centerline of the board.
        screw_distance=27;
        for(i = [ screw_distance/2,
                  -screw_distance/2 ] )
        {
            // the screw hole
            translate([i, 0,-4])
            color("white", 0.5)
            cylinder(d=1.5, h=10);

            // screwdriver access
            translate([i, 0,-104])
            color("white", 0.5)
            cylinder(d=5, h=100, $fn=16);
        }
        
        // Leave room for the lens-side soldered ends of the wires.
        // translate ([0,-12, 0])
        // color("red", 0.5)
        // ccube(12, 4, -(lens_thickness + 1));
    }
}

module sensor_access_base()
{
    // This is just the backside model, used to create an access hole.
    // the board is a 32mm circle, ~2mm thick.
    // Add some thickness to the back to leave room for components, etc.
    z = sensor_board_thickness + sensor_board_clearance;
    translate([0,0,-(lens_thickness + z)])
    {
        // The board itself
        cylinder(d=32, h=z, $fa = 5);
        
        // Hemisphere for clearance
        difference()
        {
            sphere(d=32, $fa = 5);
            
            translate([0,0,1])
            cylinder(d=32, h=17, $fa=5);
        }
    }
}

module sensor_shell()
{
    // A solid which completely encloses the sensor cutout/access cutout,
    // so it can be protected from the top.

    // This is essentially the sphere defined by sensor_access_base, expanded by 2mm.
    z = sensor_board_thickness + sensor_board_clearance;
    translate([0,0,-(lens_thickness + z)])
    {
        sphere(d=36, $fa = 5);
    }
}

module sensorpod()
{
    rotate([sensor_angle, 0, 0])
    translate([0, 0, -(lens_thickness + ball_radius + sensor_clearance)]) 
    {
        rotate([sensor_skew_angle, 0, 0])
        difference()
        {
            cylinder(d=32, h=lens_thickness);
            
            translate([-17, 2, 0]) 
            cube([34,20,lens_thickness]);
        }
//    translate([-17, -16, ball_radius + ball_clearance]) 
//    cube([34,20,4]);
    }
}

breadboard_size = [46.5, 34.75, 9.5];
breadboard_offset = [-5, -50];

module breadboard_cutout()
{
    union()
    {
        // main body
        ccube(breadboard_size[0], breadboard_size[1], breadboard_size[2]);

        // clearance cutout
        intersection()
        {
            // ccube(breadboard_size[0], breadboard_size[1], 200);
            linear_extrude(height=80, scale=1.0)
            square(center=true, [breadboard_size[0], breadboard_size[1]]);

            // Tweak the location of the top of the cutout to make it a reasonable shell
            // Note that any adjustment to the body shape may require adjusting this.
            translate([2.0, 2.0, -4.0])
            translate([-breadboard_offset[0], -breadboard_offset[1], -bottom])
            body();

        }

        // connection tabs
        translate([0, breadboard_size[1] / 2, 0])
        ccube(5.0, 5.0, 6.5);
        translate([0, -breadboard_size[1] / 2, 0])
        ccube(5.0, 5.0, 6.5);
        translate([breadboard_size[0] / 2, 0, 0])
        ccube(5.0, 5.0, 6.5);
        translate([-breadboard_size[0] / 2, 0, 0])
        ccube(5.0, 5.0, 6.5);

        // USB Cable clearance
        translate([0, 0, 4])
        rotate([0, -100, 0])
        cylinder(d=breadboard_size[1], h=50);

    }
}

module breadboard_clearance()
{

}

module body_left_cut()
{
    union()
    {
        rotate([-10, 0, 10])
        translate ([3- ball_radius, 0, bottom])
        {
            radius = 120;
            downward_curve = 40;
            ledge_height = 20;
            // translate([radius, 0, -radius])
            // cylinder(r=radius, h=radius * 2);

            translate([0, 0, ledge_height])
            rotate([0, downward_curve, 0])
            translate([radius, 0, -ledge_height])
            rotate([0, 0, 90])
            rotate_extrude(angle=180)
            intersection()
            {
                union()
                {
                    // fillet
                    fillet_amount = 20;
                    offset(-fillet_amount) 
                    offset(fillet_amount)
                    translate([radius, ledge_height])
                    rotate([0, 0, -downward_curve])
                    translate([-radius, -ledge_height])
                    union()
                    {
                        // upright wall
                        translate([-100, -100])
                        square([100 + radius, 200]);

                        // ledge
                        translate([radius, ledge_height])
                        rotate([0, 0, -15])
                        translate([-radius, -ledge_height])
                        translate([0, -100])
                        square([radius + 25, 100 + ledge_height]);

                        // Make outside edge vertical
                        translate([0, -107])
                        square([radius + 25, 100 + ledge_height]);

                    }

                    // offset(fillet_amount)
                    // offset(-fillet_amount) 
                    // square([50, 50]);
                }
                // Clip to positive X
                translate([0, -150])
                square(300, 300);
            }
        }  
        // Don't cut anything aft of tail_y_boundary with this shape.
        // translate([-100, -200 + tail_y_boundary, -100])
        // cube([200, 200, 200]);
    }
}

module body_right_front_cut()
{
    scale([0.89, 0.9, 0.815])
    rotate([0, 0, 90])
    rotate_extrude(angle=-180)
    intersection()
    {
        // scale([0.8, 0.6])
        circle(r=100);

        // Clip to positive X
        translate([0, 0])
        square(300, 300);
    }
}

module body_right_rear_cut_template(params)
{
    // Match the front right cut at the intersection point at the z=0 plane
    rotate([-params[2], 0, 0])
    scale([1, 1, 1.0 / params[3]])
    rotate([-90, 0, 0])
    {
        translate([-params[0], -params[1], 0])
        body_right_front_cut();
    }

}

module body_right_rear_cut(params)
{
    translate([params[0], params[1], 0])
    scale([1.0, params[3], 1.0])
    rotate([-params[2], 0, 0])
    union()
    {
        rotate_extrude(angle=-180)
        intersection()
        {  
            offset(-params[6]) 
            offset(params[6])
            union()
            {
                // Match the front right cut at the intersection point
                projection(cut=true)
                body_right_rear_cut_template(params);

                // add the horizontal base
                rotate([0, 0, -(params[2] + params[4])])
                translate([0, -100 + params[5]])
                square([250, 100]);
            }

            // Clip to positive X
            translate([0, -150])
            square(300, 300);
        }
        
        // Don't clip forward of this part
        translate([-200, 0, -200])
        cube([400, 400, 400]);
    }
}

module body_right_cut_translation()
{
    y_translation_angle = -45;
    z_translation_angle = 40;
    translation_amount = 80 - ball_diameter / 4;

    rotate([0, -y_translation_angle, 0])
    rotate([0, 0, -z_translation_angle])
    translate([0, 0, -translation_amount])
    rotate([0, 0, z_translation_angle])
    rotate([0, y_translation_angle, 0])
    translate([0, -30, 0])
    children();
}

// right rear shape params are:
// 0 - x offset
// 1 - y offset
// 2 - saddle rotation angle
// 3 - y scaling
// 4 - horizontal component angle (added to saddle angle)
// 5 - horizontal component height
// 6 - fillet amount
// right_rear_params = [20, 0, 35, 0.6, 5.0, 26, 60];
// right_rear_params = [20, 0, 27, 0.7, 7.5, 30, 90];
right_rear_params = [21, 2, 25, 0.7, 8, 30, 90];
// Settings roughly equivalent to previous:
// right_rear_params = [28, 0, 0, 0.7, 5.1, 22.5, 70];
module body_right_cut()
{
    body_right_cut_translation()
    intersection()
    {
        union()
        {
            body_right_front_cut();

            // Don't cut left of the center point
            translate([-200, 0, 0])
            cube([200, 200, 200]);

            // and don't cut off the tail tip
            // translate([-100, -200 - 50, -100])
            // cube([200, 200, 200]);
        }
        union()
        {
            body_right_rear_cut(right_rear_params);
        }

    }
}

module body_button_supports()
{
    for(i = button_params )
    {
        shadow_hull(10)
        button_transform(i)
        translate([0, 0, -10])
        rcube(20, 20, 10, 3);
    }

}

module body()
{
    intersection()
    {
        // Clip to above bottom surface
        translate ([0, 0, bottom])
        ccube(400, 400, 400);

        body_unclipped();
    }

}
module body_unclipped()
{
    difference()
    {
        union()
        {
            intersection()
            {
                // left cut
                body_left_cut();
                        
                // right cut
                body_right_cut();
            }        
 
            body_button_supports();

            sensor_shells();
        }

        // Trim the front of the body so it's not pointy for no reason
        rotate([0, 0, 45])
        translate([0, 100, -100 + bottom])
        ccube(100, 100, 200);
    }
}

stud_diameter = 5;
stud_height = 2;

module bottom_stud()
{
    translate([0, 0, -1])
    cylinder(d=stud_diameter, h = stud_height + 1, $fs=0.5);
}

module bottom_stud_hole()
{
    union()
    {
        bottom_stud();
        translate([0, 0, stud_height])
        sphere(d = stud_diameter, $fs=0.5);
    }
}

stud_locations = [
    // Clockwise when looking at the bottom of the body, starting from the tail.
    [0, -80],
    [25, -65],
    [33, -20],
    [-4, 23],
    [-45, 15],
    [-39, -27],
];

module bottom_cover(thickness)
{
    union()
    {
        intersection()
        {
            translate([0, 0, - bottom])
            {
                difference()
                {
                    body_unclipped();
                    ball_cutout();
                }
            }
            ccube(400, 400, -thickness);
        }

        for(i = stud_locations)
        {
            translate(i)
            bottom_stud();
        }
    }
}

module stud_hole_cutouts()
{
    for(i = stud_locations)
    {
        translate([i[0], i[1], bottom])
        bottom_stud_hole();
    }
}

module body_minimal()
{
    intersection()
    {
        // Clip to above bottom surface
        translate ([0, 0, bottom])
        ccube(400, 400, 400);
        difference()
        {
            // Main body
            union()
            {
                hull()
                {
                    // Footprint outline
                    translate ([0, 0, -(recess_radius + bottom_clearance)])
                    cylinder(h=body_height, d=body_diameter);
                    
                    // Pods for each of three support bearings
                    for(i = bearing_spacing )
                    {
                        rotate(i) 
                        rotate([bearing_angle, 0, 0])
                        translate([0, 0, -(ball_radius + (bearing_diameter/2))]) 
                        scale([1, 1, 0.5])
                        sphere(r=5);
                    }
                            
                    // Pods for two sensors
                    for(i = sensor_params )
                    {
                        rotate(i[0])
                        sensorpod();
                    }

                    // Button pods
                    for(i = button_params )
                    {
                        shadow_hull()
                        button_transform(i)
                        translate([0, 0, -10])
                        cylinder(h=10, d=button_pod_diameter);
                    }
                }
            }
        }
    }
}

wire_channel_size = 10;
wire_channel_primary_radius = 25;

module wire_cutout_profile()
{
    rotate([0, 0, 45])
    rrect(wire_channel_size, wire_channel_size, 2);
    // circle(d=wire_channel_size, $fn=30);
}

module wire_cutout_straight_section(h)
{
    translate([0, 0, -0.01])
    union()
    {
        linear_extrude(height = h)
        wire_cutout_profile();
        translate([0, 0, h])
        sphere(r=wire_channel_size * 1.25);
        // linear_extrude(height = 8, scale=2)
        // wire_cutout_profile();
    }
}

module wire_cutouts()
{
    translate([0, 0, bottom])
    union()
    {
        if(false)
        {
            // segment just to the front sensor
            // rotate([0, 0, 180 + 10])
            // translate([wire_channel_primary_radius, 0, 0])
            // rotate([90, 0, 0])
            // translate([0, 0, -0.01])
            // linear_extrude(20)
            // wire_cutout_profile();

            // rotate([0, 0, 180 + 10])
            // rotate_extrude(angle=25)
            // translate([wire_channel_primary_radius, 0, 0])
            // wire_cutout_profile();
        }
        else
        {
            // most of a full circle
            rotate([0, 0, 180 - 45])
            rotate_extrude(angle=75)
            translate([wire_channel_primary_radius, 0, 0])
            wire_cutout_profile();
        }

        rotate([0, 0, 180 + 30])
        translate([wire_channel_primary_radius, 0, 0])
        rotate([-90, 0, 0])
        wire_cutout_straight_section(20);

        rotate([0, 0, -30])
        translate([wire_channel_primary_radius, 0, 0])
        rotate([90, 0, 0])
        wire_cutout_straight_section(20);

        rotate([0, 0, -35])
        rotate_extrude(angle=63)
        translate([wire_channel_primary_radius, 0, 0])
        wire_cutout_profile();

        // Access out the front for the USB cable.
        // Match the radius of the outer edge.
        secondary_radius = 70;
        rotate([0, 0, 27])
        translate([wire_channel_primary_radius - secondary_radius, 0, 0])
        rotate_extrude(angle=55)
        translate([secondary_radius, 0, 0])
        wire_cutout_profile();

        // Room for the breadboard under the handrest
        translate([breadboard_offset[0], breadboard_offset[1], 0])
        breadboard_cutout();
    }

}

module sensor_transform(params)
{
    rotate([0, 0, params[0]])
    rotate([sensor_angle, 0, 0])
    translate([0, 0, -(ball_radius + sensor_clearance)]) 
    rotate([sensor_skew_angle, 0, 0])
    children();
}

module sensor_cutouts()
{
    for(params = sensor_params )
    {
        difference()
        {
            sensor_transform(params)
            sensor_cutout();
            
            // // Clip the cutouts to the top of the base ring.
            // translate([0, 0, body_height + bottom])
            // ccube(100, 100, -100);
        }
    }
}

module sensor_access_cutouts()
{
    for(params = sensor_params )
    {
        if (params[1])
        {
            // Bottom access cutout
            shadow_hull()
            sensor_transform(params)
            sensor_access_base();
        }
    }
}

module sensor_shells()
{
    for(params = sensor_params )
    {
        difference()
        {
            shadow_hull()
            sensor_transform(params)
            sensor_shell();
        }
    }
}


module button_cutouts()
{
    for(params = button_params )
    {
        // cutout for the button itself
        button_transform(params)
        button_cutout(params[6], params[7]);

        // Bottom access cutout for the button
        shadow_hull()
        button_transform(params)
        button_access_base();

    }
}

$fa = 4;

module full()
{
    intersection() 
    {
        difference()
        {
            body();
            ball_cutout();
            bearing_cutouts();
            sensor_cutouts();
            sensor_access_cutouts();
            button_cutouts();
            wire_cutouts();
            stud_hole_cutouts();
        }

        // Isolate bearing hole for testing purposes
    //    translate([0, 20, -10]) cube(center = true, [10, 20, 20]);
    }
}

full();

