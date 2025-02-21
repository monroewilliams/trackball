include <BOSL2/std.scad>


// Examples
//switch_carrier(20, 0, 14);
//
//switch_carrier(5, 20, 14);
//
//switch_carrier(5, 5);

$fa = 1;
$fs = 0.01;


module switch_carrier(front_overhang = 0, rear_overhang = 0, body_height = 14, bridge = true, split = true, hinge = false)
{
    // front_overhang and rear_overhang extend the top surface on the end opposite the hinge, and the end with the hinge, respectively.

    // This makes a cutout that leaves room to plug a 3-position JST-XH connector onto the
    // two "normally open" pins of the switch.
    connector_space = true;

        
    // Dimensions of the microswitch. I'm using OMRON D2FC-F-7N switches.
    // Note that these dimensions are likely to be finicky and dependent on the accuracy of your printer.
    switch_length = 13;
    switch_width = 6;
    switch_height = 6.8;

    // This is the thickness of the top over the hinge cut. I recommend tweaking this to make your
    // slicer lay this down as a single line for flexibility. When slicing with Cura, if I make it
    // too thin that part of the model actually gets omitted, and if I make it too thick it gets
    // messy as well.
    hinge_thickness = 0.4;

    // Outer size of the main body of the adapter. Note that "height" is a bit of a misnomer,
    // as the top deck extends well beyond this.
    // The width may need fine adjustment based on your printer and the size of the hole you're
    // fitting the switch into. This setting works well for my printer, when using a hole that
    // I've modeled as exactly 14x14mm.
    body_width = 13.8;
    
    // Reduce the overall width of the body slightly in one direction, to make for an easier fit.
    body_clearance = 0.4;

    switch_centering = (body_width - switch_length) / 2;

    top_thickness = 2;
    top_safety = front_overhang >= 1;

//    $fs=0.5;

    top_rotation_base = switch_centering + switch_length - 1;

    // Test cube to help me see where the rotation point of the top is
    //translate([top_rotation_base + rear_overhang, body_height, 5])
    //cube([10, 10, 10]);

    // Length of the top, not counting rounded ends
    top_length = body_width + front_overhang + rear_overhang;
    // Final X offset the top will be shifted by
    top_offset =  switch_centering - front_overhang - 2;

    // distance from the left edge of the switch to the center of the contact
    contact_offset = 4.5;
    // space we want to leave under the contact at rest
    contact_space = 1.5;
    // Calculate a top angle that will leave enough space under the center of the contact at rest.
    contact_distance = (top_rotation_base + rear_overhang) - (switch_centering + 4.5);
    top_angle =  atan(contact_space/(rear_overhang + body_width + top_thickness));

    module microswitch_cut()
    {
        difference()
        {
            union()
            {
                // Main switch body
                cuboid([switch_length, switch_height, switch_width], anchor = FRONT);
                
                // clearance for pins
                for (i = [-5, 0, 5 ])
                translate([i, 0, 0])
                cuboid([2, 5, switch_width / 6], anchor = BACK);
                
                // the button itself
                color("red")
                translate([4.5 -(switch_length/2), switch_height, 0])
                rotate([0, 0, 90])
                cylinder(d = 1, h = 3, center = true);
                
            }
            union()
            {
                // nubs that fit the holes in the switch
                r = 1.3 / 2;
                for (z = [-switch_width/2, switch_width/2])
                for (x = [-2.7, 2.7])
                translate([x, 1.0 + r, z])
                {
                    sphere(r = r - 0.1);
                }
            }
        }
    }

    module sh_xt_3p_cut()
    {
        translate([(8.5 - switch_length) / 2, 0,  4 - (switch_width / 2)])
        {
            cuboid([8.5, 10, 4], anchor = BACK + TOP);
            
            // cuts for the key
            translate([-3.5 / 2, 0, 0])
            cube([1.75, 10, 1], anchor = BACK + BOTTOM + RIGHT);
            translate([3.5 / 2, 0, 0])
            cube([1.75, 10, 1], anchor = BACK + BOTTOM + LEFT);
            translate([0, -10, 0])
            cube([1.75 + 3 + 1.75, 3, 1], anchor = FRONT + BOTTOM);
        }
    }

    
    module switch_cutout()
    {
        difference()
        {
            translate([0, 0, 0])
            {
                // clearance above the switch button
                translate([0, switch_height - 0.4, 0])
                cuboid([switch_length - 2, 3.0, body_width - 4], anchor = FRONT,
                chamfer = 1.4, edges = "Y");
                
                if (!bridge)
                {
                    cube([switch_length, switch_height, body_width]);
                }
                
                // pushout hole
                translate([0, switch_height / 2, 0])    
                cyl(d=3, h=body_width, anchor = TOP);

                // This includes the switch body, but also the contact cutouts
                microswitch_cut();
                
                // Cut above the switch body, but only on the top piece
                translate([0, 4, switch_width - 2])
                cuboid([switch_length, switch_height - 4, switch_width], anchor = FRONT,
                    chamfer = 2, edges = "Y", except = BOTTOM);
                
            }
            // chamfer the inside corners, but leave room to get the switch out past the supports.
//            translate([0, 0, switch_width + ((switch_width / 2) - 1)])
//            {
//                translate([-switch_length/2, 0, 0])
//                rotate([0, -45, 0])
//                cube([5, body_height, 5]);
//
//                translate([switch_length/2, 0, 0])
//                rotate([0, -45, 0])
//                cube([5, body_height - 1, 5]);
//            }
        }
        
    }

    module body()
    {
        body_offset = [-body_width/2, - body_height + (switch_height + 1.2), -body_width/2];
        difference()
        {
            union()
            {
                // main body
                translate(body_offset - [0, 0, 0])
                cuboid([body_width, body_height, body_width - body_clearance], anchor = [-1, -1, -1],
                    rounding = 2, except = BACK);
            
                // lip
                translate(body_offset)
                difference()
                {
                    translate([-1, body_height - 4, 0])
//                    cube([body_width + 2, 4, body_width + 1]);
                    cuboid([body_width + 2, 4, body_width + 1], anchor = [-1, -1, -1],
                        rounding = 2, edges = "Y");

                    if (top_safety)
                    {
                        translate([-3, body_height - 2.25, 0])
                        cylinder(d=5.25, h=body_width + 1);
                    }
                }
                
                // Extension for hinge
                if (rear_overhang > 0)
                {
                    translate(body_offset)
                    translate([top_rotation_base + top_thickness, body_height, 0])
                    cuboid([rear_overhang, 3, body_width], anchor = [-1, 1, -1],
                        rounding = top_thickness/2, 
                        edges = [TOP+RIGHT, BOTTOM+RIGHT, RIGHT+FRONT]);
                }
                
                // body side of safety catch
                if (top_safety)
                {
                    translate(body_offset)
                    translate([-2.25, body_height - .75, 0])
                    {
                        cube([2.25, .75, body_width]);
                        translate([.25, 0, 0])
                        cylinder(d=1, h=body_width, $fn=3);
                    }
                }
            }
            union()
            {
                // cut for clearance on extended hinge
                translate(body_offset)
                if (rear_overhang > 0)
                {
                    translate([top_rotation_base + 1, body_height, 0])
                    cuboid([rear_overhang - 1, 1, body_width + 1], anchor = [-1, 1, -1]);
                }
                
                if (connector_space)
                {
                    sh_xt_3p_cut();
                }
            }
        }

//        translate([(body_width - switch_length)/2, 5, (body_width - switch_width)/2])
//        sh_xt_3p_cut();

    }

    module top()
    {
        translate([top_length, 0, 0])
        translate([top_offset - (body_width / 2), switch_height + 1.2, 0])
//        translate([top_length, 0, 0])
        rotate([0, 0, -top_angle]) 
//        translate([-top_length, 0, 0])
        {
            {
//                translate([top_length, 0, 0])
                difference()
                {
                    union()
                    {
                        // main top surface (adding top_thickness/2 to match the radius of the original cylinder)
                        cuboid([top_length + top_thickness/2, top_thickness, body_width], anchor = RIGHT + FRONT,
                            rounding = top_thickness/2, except = [RIGHT, FRONT+TOP, FRONT+BOTTOM]);

                        
                        // Rounded hinge end
                        cyl(r=top_thickness, h=body_width, anchor = CENTER, rounding = 1);
                        
                        // Add a bit more around the hinge to help it print in the first layer
                        translate([hinge_thickness - (top_thickness), 0, 0])
                        cylinder(r=top_thickness, h=body_width / 2, anchor = TOP);//, $fn=6);

                    }
                    
                    union()
                    {
                        // cut bottom half of hinge end round and hinge reinforcement
                        cube([top_thickness * 4, top_thickness, body_width], anchor = BACK);

                        // hinge cut
                        translate([hinge_thickness - (top_thickness), 0, 0])
                        cylinder(r=top_thickness - hinge_thickness, h=body_width, anchor = CENTER, $fn=6);
                    }
                }
                
                // switch contact
                // This wants to be centered on a point contact_offset from the left end of the switch.
                contact_width = 3;
                translate([-top_length, 0, 0])
                translate([-top_offset + switch_centering + contact_offset, 
                    0,
                    0])
                cuboid([contact_width, 1.2, switch_width], anchor = BACK,
                    chamfer = 1.2, edges = [FRONT+TOP, FRONT+BOTTOM]);
                
                if (top_safety)
                {
                    translate([-top_length, 0, 0])
                    translate([-top_offset -3.5, -3.5, 0])
                    {
                        difference()
                        {
                            // downward stem
                            intersection()
                            {
                                translate([-1, -.25, 0])
                                cube([2.25, 3.25 + top_thickness/2, body_width], anchor = [-1, -1,0]);
                                
                                translate([3, 2, 0])
                                cylinder(d=7, h=body_width, anchor = CENTER);
                            }
                            
                            // clearance cut
                            translate([3, 2, 0])
                            cylinder(d=5.25, h=body_width, anchor = CENTER);

                        }
                        
                        // cross piece
                        translate([.5, -.25, 0])
                        cube([1.75, .75, body_width], anchor = [-1, -1,0]);
                        
                        // upward-facing tooth
                        translate([2, .5, 0])
                        rotate([0, 0, 180])
                        cylinder(d=1, h=body_width, anchor = CENTER, $fn=3);
                    }
                }
                
            }
        }
    }
    
    module split_cut(side)
    {
        if (hinge)
        {
            hinge_diameter = 6;
            clearance1 = ((side == 1)?0.2:0);
            clearance2 = ((side == 2)?0.2:0);
            union()
            {
                difference()
                {
                    cuboid([14, body_height - 4, body_width], anchor = FRONT+BOTTOM);
                    translate([2, -0.2, 0])
                    rotate([0, 90, 0])
                    cylinder(d = hinge_diameter - clearance1, h = 2);
                }

                intersection()
                {
                    union()
                    {
                        translate([3 + (clearance2/2) - (clearance1/2), -0.2, 0])
                        rotate([0, 90, 0])
                        cylinder(d = hinge_diameter - clearance2, h = 1.0 + (clearance1 * 2));

                        // anchor the pivot to the top side
                        translate([3 + (clearance2/2), 0, 0])
                        cuboid([1, hinge_diameter / 2, hinge_diameter / 2],
                            anchor = LEFT + FRONT + BOTTOM);
                                    
                        // the hinge pivot
                        translate([3.5, -0.2, 0])
                        sphere(r=1.5 + clearance1);
                           
                    }

                    intersection()
                    {
                        translate([3, -0.2, 0])
                        rotate([0, 90, 0])
                        cylinder(d = hinge_diameter, h = 2 + clearance1, center = true);

                        if (side == 2)
                        {
                            // Chamfer the outside of the sticking-out part
                            translate([3, 1 + hinge_diameter/2, 0])
                            cuboid([2, hinge_diameter, hinge_diameter], anchor = BACK
                                , chamfer = 2,
                                edges = [FRONT+BOTTOM, FRONT + TOP]
                            );
                        }
                    }
                }

                if (side == 1)
                {
                    // Chamfer the outside of the sticking-out part
                    difference()
                    {
                        translate([3, 0, 0])
                        cuboid([10, 10, 10], anchor = BACK);
                        intersection()
                        {
                            translate([3, -0.2, 0])
                            rotate([0, 90, 0])
                            cylinder(d = hinge_diameter - clearance1, h = 2, center = true);
                            // Chamfer the bottom, for printability
                            translate([3, 1 + hinge_diameter/2, 0])
                            cuboid([2, hinge_diameter, hinge_diameter], anchor = BACK
                                , chamfer = 2,
                                edges = [FRONT+BOTTOM, FRONT + TOP]
                            );
                        }
                    }
                }
            }
            
        }
        else
        {
            union()
            {
//                translate([0, switch_height - 2.8, 0])
                cuboid([14, body_height - 4, body_width], anchor = FRONT+BOTTOM);

                translate([2, 0.5, 0])
                cuboid([2, body_height - 8.5, 3.5], anchor = FRONT+LEFT+TOP);

            }
        }
    }

    module cut_switch()
    {
        // Put the bottom of the switch exactly at the Y origin
        {
//            color("green", 0.25)
//            split_cut(1);

            difference()
            {
                union()
                {
                    translate([0, -(- body_height + (switch_height + 1.2)), 0])
                    children();

                    if (hinge)
                    intersection()
                    {
//                        color("blue", 0.5)
                        translate([3, 0, 0])
                        cuboid([2, 5, 7], anchor = BACK
//                            , chamfer = 1.5, edges = FRONT+BOTTOM
                        );

                        // Use a section of a larger cylinder here, for a lower profile
                        // and also for printability
//                        translate([3, 1.3, 0])
//                        rotate([0, 90, 0])
//                        cylinder(d = 6.5, h = 2, center = true);
                    }
                }
                split_cut(1);
            }
            
            // Separate the halves -- useful when working on the hinge design
//            translate([0, -6, 0])

            translate([0, hinge?-0.4:-6, -body_clearance])
            rotate([180, 0, 0])
            {
//                color("green", 0.25)
//                split_cut(2);
                intersection()
                {
                    union()
                    {
                        translate([0, -(- body_height + (switch_height + 1.2)), 0])
                        children();
                        if (hinge)
                        intersection()
                        {
//                            color("blue", 0.5)
                            translate([3, 0, 0])
                            cuboid([2, 5, 7], anchor = BACK
//                                , chamfer = 1.5, edges = FRONT+TOP
                            );
                            // Use a section of a larger cylinder here, for a lower profile
                            // and also for printability
//                            translate([3, 1.3, 0])
//                            rotate([0, 90, 0])
//                            cylinder(d = 6.5, h = 2, center = true);
                        }
                    }
                    split_cut(2);
                }
            }
        }
    }

    module assembly()
    {
        difference()
        {
            union()
            {
                body();
            }
            
            switch_cutout();
        }        

        top();
    }
    
    if (split)
    {
        // Cut the switch into its printable parts
        cut_switch() assembly();

//        microswitch_cut();
//        sh_xt_3p_cut();

    }
    else
    {
        // Just show the assembled switch, which is not printable/usable
        assembly();
    }
}