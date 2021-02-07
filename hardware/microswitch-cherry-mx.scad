// These extend the top surface on the end opposite the hinge, and the end with the hinge, respectively.
top_extra_overhang = 1;
top_extra_hinge = 0;

// This makes a cutout that leaves room to plug a 3-position JST-XH connector onto the
// two "normally open" pins of the switch.
connector_space = false;

// Outer size of the main body of the adapter. Note that "height" is a bit of a misnomer,
// as the top deck extends well beyond this.
// The width may need fine adjustment based on your printer and the size of the hole you're
// fitting the switch into. This setting works well for my printer, when using a hole that
// I've modeled as exactly 14x14mm.
body_width = 13.8;
body_height = 10;

// Dimensions of the microswitch. I'm using OMRON D2FC-F-7N switches.
// Note that these dimensions are likely to be finicky and dependent on the accuracy of your printer.
switch_length = 12.8;
switch_width = 6;
switch_height = 6.8;

// This is the thickness of the top over the hinge cut. I recommend tweaking this to make your
// slicer lay this down as a single line for flexibility. When slicing with Cura, if I make it
// too thin that part of the model actually gets omitted, and if I make it too thick it gets
// messy as well.
hinge_thickness = 0.18;

top_thickness = 2;
top_safety = top_extra_overhang >= 1;

$fs=0.5;

switch_bottom_height = body_height - (switch_height + 1.2);
switch_centering = (body_width - switch_length) / 2;
top_rotation_base = switch_centering + switch_length - 1;

// Test cube to help me see where the rotation point of the top is
//translate([top_rotation_base + top_extra_hinge, body_height, 5])
//cube([10, 10, 10]);

// Length of the top, not counting rounded ends
top_length = body_width + top_extra_overhang + top_extra_hinge;
// Final X offset the top will be shifted by
top_offset =  switch_centering - top_extra_overhang - 2;

// distance from the left edge of the switch to the center of the contact
contact_offset = 4.5;
// space we want to leave under the contact at rest
contact_space = 1.5;
// Calculate a top angle that will leave enough space under the center of the contact at rest.
contact_distance = (top_rotation_base + top_extra_hinge) - (switch_centering + 4.5);
top_angle =  atan(contact_space/(top_extra_hinge + body_width + top_thickness));

module switch_cutout()
{
    difference()
    {
        translate([0, 0, 0])
        {
            // main switch body
            translate([0, switch_bottom_height, 0])
            cube([switch_length, switch_height, body_width]);
            // top clearance
            translate([1, switch_bottom_height + switch_height - 0.4, -body_width])
            cube([switch_length - 2, 3.0, body_width * 2]);
            // pushout hole
            translate([
                switch_length / 2, 
                switch_bottom_height + switch_height / 2, 
                -body_width
            ])    
            cylinder(d=3, h=body_width * 2);
            
            translate([0, -1, 0])
            {
                translate([0, 0, 1])
                {
                    // Cutouts for contacts
                    cube([2.5, 5, 13]);
                    translate([switch_length - 2.5, 0, 0])
                    cube([2.5, 5, 13]);
                    translate([(switch_length - 2.5) / 2, 0, 0])
                    cube([2.5, 5, 13]);
                }

                // The bottom supports don't need to extend past the center of the switch.
                translate([2, 0, (switch_width / 2) - 1])
                cube([9, 5, body_width - 1]);
                
            }
        }
        // chamfer the inside corners, but leave room to get the switch out past the supports.
        translate([0, 0, switch_width + ((switch_width / 2) - 1)])
        {
            rotate([0, -45, 0])
            cube([5, body_height, 5]);

            translate([switch_length, 0, 0])
            rotate([0, -45, 0])
            cube([5, body_height, 5]);
        }
    }
    
}

module body()
{
    difference()
    {
        union()
        {
            // main body
            // Instead of this, give the body slightly rounded outside corners.
            // cube([body_width, body_height, body_width]);
            rotate([-90, 0, 0])
            translate([0, -body_width, 0])
            linear_extrude(body_height)
            offset(1, $fn=32) offset(delta=-1)
            square(body_width, body_width);
        
            // lip
            difference()
            {
                translate([-1, 6, 0])
                cube([body_width + 2, body_height - 6, body_width + 1]);

                if (top_safety)
                {
                    translate([-3, body_height - 2.25, 0])
                    cylinder(d=5, h=body_width + 1, $fn=32);
                }
            }
            
            // Extension for hinge
            if (top_extra_hinge > 0)
            {
                translate([top_rotation_base + top_thickness, body_height - 3, 0])
                cube([top_extra_hinge, 3, body_width]);
            }
            
            // body side of safety catch
            if (top_safety)
            {
                translate([-2.25, body_height - .75, 0])
                {
                    cube([2.25, .75, body_width]);
                    translate([.25, 0, 0])
                    cylinder(d=1, h=body_width, $fn=3);
                }
            }
        }
        // cut for clearance on extended hinge
        if (top_extra_hinge > 0)
        {
            translate([top_rotation_base, body_height - 1, 0])
            cube([top_extra_hinge, 1, body_width + 1]);
        }
        
        if (connector_space)
        {
            translate([0, 0, 1.0 + (body_width - switch_width)/2])
            cube([8.5, switch_bottom_height, body_width + 1]);
        }
    }

}

module top()
{
    translate([0, body_height, 0])
    translate([top_offset, 0, 0])
    translate([top_length, 0, 0]) rotate([0, 0, -top_angle]) translate([-top_length, 0, 0])
    {
        {
            difference()
            {
                union()
                {
                    // main top surface
                    cube([top_length, top_thickness, body_width]);

                    // Rounded free end
                    translate([0, top_thickness / 2, 0])
                    cylinder(d=top_thickness, h=body_width);

                    // Rounded hinge end
                    translate([top_length, 0, 0])
                    cylinder(r=top_thickness, h=body_width);

                }
                
                // cut bottom half of hinge end round
                translate([top_length - top_thickness, -top_thickness, 0])
                cube([top_thickness * 2, top_thickness, body_width]);

                // hinge cut
                translate([top_length + hinge_thickness - (top_thickness), 0, 0])
                cylinder(r=top_thickness - hinge_thickness, h=body_width, $fn=6);
            }
            
            // switch contact
            // This wants to be centered on a point contact_offset from the left end of the switch.
            contact_width = 3;
            translate([-top_offset + switch_centering + contact_offset - contact_width / 2, 
                -1.2,
                0])
            cube([3, 2.4, body_width]);
            
            if (top_safety)
            {
                translate([-top_offset -3.5, -3.5, 0])
                {
                    difference()
                    {
                        // downward stem
                        intersection()
                        {
                            translate([-1, 0, 0])
                            cube([2.25, 3.25 + top_thickness/2, body_width]);
                            
                            translate([3, 2, 0])
                            cylinder(d=7, h=body_width, $fn=32);
                        }
                        
                        // clearance cut
                        translate([3, 2, 0])
                        cylinder(d=5, h=body_width, $fn=32);

                    }
                    
                    // cross piece
                    translate([.5, 0, 0])
                    cube([1.75, .75, body_width]);
                    
                    // upward-facing tooth
                    translate([2, .75, 0])
                    rotate([0, 0, 180])
                    cylinder(d=1, h=body_width, $fn=3);
                }
            }
            
        }
    }
}

// Assemble the pieces
difference()
{
    union();
    {
        body();
    }
    
    translate([switch_centering, 0, (body_width - switch_width)/2])
    switch_cutout();
}
top();
