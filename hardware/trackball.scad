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
    [100, 28, 12, -3, 11, -4 - 90, 5, 15],
    [-60, 38, 11, 20, -5, 5 + 90, 5, 5]
];

module button_transform(params)
{
    rotate([0, 0, params[0]])
    rotate([-params[1], 0, 0])
    translate([0, ball_radius + params[2], 0])
    rotate([0, params[4], 0])
    rotate(-[params[3], 0, 0])
    rotate([0, 0, params[5]]) 
    children();
}

button_pod_diameter = 20;

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

module shadow_hull()
{
    // This creates an object which is a hull of the object itself and its shadow on the x/y plane.
    hull()
    {
        // the child object
        children();

        // the projection of the child object
        translate([0, 0, bottom - 1]) 
        linear_extrude(height=1)
        projection()
        children();
    }
}

module ball()
{
    color("red") sphere(d=ball_diameter);
}

module ball_cutout()
{
    union()
    {
        // spherical part of the recess
        //sphere(d=50, $fa=5, $fs=0.1);
        sphere(d=ball_diameter + (ball_clearance * 2));

        // top straight part
        // rotate([0, 0, 10])
        // rotate([0, 35, 0])
        // cylinder(h=30, r = recess_radius);

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

        // 2mm cut around the hole for seating/overhang
        ccube(20, 20, 40);

        // Front and rear overhangs are intended to match up with the overhang parameters in 
        // microswitch-cherry-mx.scad.
        if (front_overhang > 0)
        {
            translate([-10, 10, 0])
            cube([20, front_overhang + 5, 6]);
        }

        if (rear_overhang > 0)
        {
            translate([-10, -10 -rear_overhang, 0])
            cube([20, rear_overhang, 6]);
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
            cylinder(d=5, h=100);
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

tail_y_boundary = -80;

module body_tail_cut()
{
    translate([0, 0, bottom])
    linear_extrude(height=-bottom, scale=[1.4,1])
    {  
        intersection()
        {
            offset(-50) offset(50) 
            union()
            {
                // match the left cut curve. This requires some tweaking.
                cut_radius = 176;
                // color("blue", 0.25)
                translate([155 + 3 - ball_radius, 3])
                circle(r=cut_radius);

                // Make a bit of a tail
                translate([0, 27 - cut_radius])
                circle(r = 20);
            }
            // Don't cut anything forward of tail_y_boundary with this shape.
            translate([-100, -200 + tail_y_boundary])
            square([200, 200]);

        }
    }
}

module body_left_cut()
{
    difference()
    {
        rotate([-10, 0, 0])
        translate ([3- ball_radius, 0, bottom])
        {
            radius = 120;
            downward_curve = 30;
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
                        // square([100, 3]);
                        // translate([0, -bottom])
                        // circle(d=60);
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
        // // Don't cut anything aft of tail_y_boundary with this shape.
        // translate([-100, -200 + tail_y_boundary, -100])
        // cube([200, 200, 200]);
    }
}

module body_right_cut()
{
    radius = 150;
    rotate([0, -45, 40])
    difference()
    {
        translate([ball_diameter / 4 + -radius, 0, 0])
        sphere(r=radius);

        rotate([0, 90, 0])
        cylinder(d=ball_diameter, h=ball_diameter);

    }


}

module body_back_cut()
{
    translate([0, 0, bottom])
    rotate_extrude(angle=360)
    intersection()
    {
        translate([0, 0, 0])
        {
            // fillet
            fillet_amount = 40;
            offset(-fillet_amount) 
            offset(fillet_amount)
            union()
            {
                rotate([0, 0, -4])
                square([200, 15]);
                // square([50, 25]);
                translate([0, 5])
                circle(r=60);
            }

            // offset(fillet_amount)
            // offset(-fillet_amount) 
            // square([50, 50]);
        }
        // Clip to positive X
        translate([0, 0])
        square(300, 300);
    }

}

module body()
{
    difference()
    {
        // color("white", 0.75)
        intersection()
        {
            // Clip to above bottom surface
            translate ([0, 0, bottom])
            ccube(400, 400, 400);

            union()
            {
                // left cut
                body_left_cut();
                // tail cut
                body_tail_cut();
            }
                    
            // right cut
            body_right_cut();

            // back cut
            body_back_cut();
        
        }
    }

}

module body_minimal()
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

module wire_cutouts()
{
    // wire routing channels
    wire_channel_size = 10;
    translate([0, 0, bottom])
    rotate_extrude()
    translate([27, 0, 0])
    circle(d=wire_channel_size, $fn=30);
    // rotate([0, 0, 45])
    // rrect(wire_channel_size, wire_channel_size, 0);

    rotate([0, 0, 55])
    translate([0, ball_radius + 15, bottom - 1])
    sphere(r=20);
    // cylinder(r=15, h=15);

    rotate([0, 0, 35])
    translate([0, ball_radius + 40, bottom - 1])
    sphere(r=20);

    // rotate([0, 0, 57])
    // translate([0, 27, bottom])
    // rotate([-90, 0, 0])
    // linear_extrude(height=60)
    // rotate([0, 0, 45])
    // rrect(wire_channel_size, wire_channel_size, 0);

    // rotate([0, 0, -95])
    // translate([-20, 27, bottom])
    // rotate([0, -90, 0])
    // linear_extrude(height=60)
    // rotate([0, 0, 45])
    // rrect(wire_channel_size, wire_channel_size, 0);

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
            
            // Clip the cutouts to the top of the base ring.
            translate([0, 0, body_height + bottom])
            ccube(100, 100, -100);
        }

        if (params[1])
        {
            // Bottom access cutout
            shadow_hull()
            sensor_transform(params)
            sensor_access_base();
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
            button_cutouts();
            wire_cutouts();
        }

        // Isolate bearing hole for testing purposes
    //    translate([0, 20, -10]) cube(center = true, [10, 20, 20]);
    }
}

full();

