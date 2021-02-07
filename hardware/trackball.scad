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
sensor_skew_angle=10;
sensor_spacing = [
    [0, 0, 180],
    [0, 0, 45]
];
sensor_clearance=2;
lens_thickness=3.3;
sensor_board_thickness=2;
sensor_board_clearance=0;
        

// Give the ball clearance all the way around in the recess
ball_radius=ball_diameter/2;
recess_radius=ball_radius + ball_clearance;

// Button params are:
// - location angle (clockwise from top)
// - distance from center
// - tilt angle
// - cutout rotation
button_params = [
    [180 + 10, 40, 30, 0],
    [30,       40, 30, 45]
];

button_pod_diameter = 20;


module ball()
{
    color("red") sphere(d=ball_diameter);
}

module ball_and_bearings_cutout()
{
    // render()
    union()
    {
        // spherical part of the recess
        //sphere(d=50, $fa=5, $fs=0.1);
        sphere(d=ball_diameter + (ball_clearance * 2));

        // top straight part
        // rotate([0, 0, 10])
        // rotate([0, 35, 0])
        // cylinder(h=30, r = recess_radius);

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

module ccube(x, y, z)
{
    translate([0, 0, z/2])
    cube(center=true, [x,y, abs(z)]);
}

module rrect(x, y, r)
{
    translate([-x/2, -y/2, 0])
    translate([r, r, 0])
    offset(r, $rs=30)
    square([x - (r * 2), y - (r * 2)]);
}

module key_cutout()
{
    translate([0, 0, 0])
    union() 
    {
        
        translate([0, 0, -20])
        {
            ccube(14, 14, 40);
            // rotate([0, 0, 0])
            // translate([20, 0, 0])
            // ccube(40, 3, 20);
        }

        ccube(20, 20, 40);
    }
}

module sensor_cutout()
{
    // render()
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
            cylinder(d=5, h=100);
        }
        
        // Leave room for the lens-side soldered ends of the wires.
        // translate ([0,-12, 0])
        // color("red", 0.5)
        // ccube(12, 4, -(lens_thickness + 1));
    }
}

module sensor_access_cutout()
{
    // This is just the backside model, used to create an access hole.
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

module handrest()
{
    // color("white", 0.75)
    difference()
    {
        intersection()
        {
            // bottom surface
            translate ([0, 0, -(recess_radius + bottom_clearance)])
            ccube(300, 300, 300);

            // left cut
            // rotate([0, 5, -10])
            // translate([100, 0, 0])
            // sphere(d=250);
            rotate([0, 5, -10])
            translate([124, 0, 0])
            cylinder(center=true, d=300, h=250);
            
            // right cut
            rotate([0, -45, 10])
            translate([-65, 0, 0])
            sphere(d=150);

            // back cut
            rotate([25, 30, 0])
            translate([0, 0, -60])
            sphere(d=175);
        
            // // back cut 2
            // // color("blue", 0.5)
            // rotate([30, 45, 0])
            // translate([0, 0, -60])
            // sphere(d=175);

        }
        // translate ([0, -(ball_radius + sensor_clearance), -(recess_radius + bottom_clearance)])
        // rotate([0, 90, 0])
        // color("blue", 0.5)
        // cylinder(center=true, r=30, h=100);
    }
}

module body()
{
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
                for(i = sensor_spacing )
                {
                    rotate(i)
                    sensorpod();
                }

                // Button pods
                for(i = button_params )
                {
                    rotate([0, 0, i[0]])
                    translate ([i[1], 0, -(recess_radius + bottom_clearance)])
                    difference()
                    {
                        cylinder(h=25, d=button_pod_diameter);
                        translate([0, 0, 12.5])
                        rotate([0, i[2], 0]) 
                        rotate([0, 0, i[3]]) 
                        ccube(40, 40, 40);
                    }
                }
            }

            handrest();
        }
        
        // bottom hole
        translate ([0, 0, -50])
        cylinder(h=100, d = hole_diameter);
                
        // wire routing tunnel
        wire_channel_size = 8;
        translate([0, 0, -(recess_radius + bottom_clearance)])
        rotate_extrude()
        translate([27, 0, 0])
        rotate([0, 0, 45])
        rrect(wire_channel_size, wire_channel_size, 0);

        rotate([0, 0, 57])
        translate([0, 27, -(recess_radius + bottom_clearance)])
        rotate([-90, 0, 0])
        linear_extrude(height=60)
        rotate([0, 0, 45])
        rrect(wire_channel_size, wire_channel_size, 0);

        rotate([0, 0, -95])
        translate([-20, 27, -(recess_radius + bottom_clearance)])
        rotate([0, -90, 0])
        linear_extrude(height=60)
        rotate([0, 0, 45])
        rrect(wire_channel_size, wire_channel_size, 0);
    }

}

module sensor_cutouts()
{
    // two sensors
    for(i = sensor_spacing )
    {
        difference()
        {
            rotate(i)
            rotate([sensor_angle, 0, 0])
            translate([0, 0, -(ball_radius + sensor_clearance)]) 
            rotate([sensor_skew_angle, 0, 0])
            sensor_cutout();
            
            // Clip the cutouts to the top of the base ring.
            translate([0, 0, body_height-(recess_radius + bottom_clearance)])
            ccube(100, 100, -100);
        }
    }

    // Bottom access for sensor #1
    color("blue")
    hull()
    {
        rotate(sensor_spacing[0])
        rotate([sensor_angle, 0, 0])
        translate([0, 0, -(ball_radius + sensor_clearance)]) 
        rotate([sensor_skew_angle, 0, 0])
        sensor_access_cutout();
        
        translate([0, 0, -(recess_radius + bottom_clearance + 1)]) 
        linear_extrude(height=1)
        projection()
        rotate(sensor_spacing[0])
        rotate([sensor_angle, 0, 0])
        translate([0, 0, -(ball_radius + sensor_clearance)]) 
        rotate([sensor_skew_angle, 0, 0])
        sensor_access_cutout();
    }

}

module key_cutouts()
{
        // Button cutouts
        for(i = button_params )
        {
            rotate([0, 0, i[0]])
            translate([i[1], 0, 12.5 -(recess_radius + bottom_clearance)])
            rotate([0, i[2], 0]) 
            rotate([0, 0, i[3]]) 
            key_cutout();
        }
}

$fa = 4;

intersection() 
{
    // render()
    difference()
    {
        body();

        ball_and_bearings_cutout();
        
        sensor_cutouts();
        key_cutouts();
    }

    // Isolate bearing hole for testing purposes
//    translate([0, 20, -10]) cube(center = true, [10, 20, 20]);
}

//sensors();
//  ball();

// handrest();
