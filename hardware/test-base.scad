use <trackball.scad>
// Test base for recessed button cutouts.

$fn=32;

module button_test_base(front_overhang, back_overhang, count = 1)
{
    difference()
    {
        border = 2;
        square_size = 20;
        width = border + (count * (border + square_size));

        front_extra = ((front_overhang > 0)?(front_overhang + 3):(3));
        back_extra = ((back_overhang > 0)?(back_overhang + 1):(3));
        length = border + front_extra + 14 + back_extra + border;
        offset_adjustment = (front_extra - back_extra) / 2;

        // main body
        translate([0, offset_adjustment, 0])
        rcube(width, length, 16, 2);

        // This cuts away too much material, and probably won't work well.
//        if (count > 1)
//        {
//            // crossways wire tunnel
//            cross_tunnel_length = square_size + (count - 1) * (square_size + border);
//            rotate([0, 90, 0])
//            rotate([0, 0, 45])
//            translate([0, 0, -cross_tunnel_length/2])
//            rcube(6, 6, cross_tunnel_length, 1);
//            
//        }
                
        for(i = [0:count - 1])
        {
            
            translate([-(width / 2) + border + square_size / 2+ i * (border + square_size), 0, 0])
            {
                translate([0, 0, 10])
                {
                    // switch cutout
                    key_plug_base();
//                    button_cutout(front_overhang, back_overhang);
//                    shadow_hull()
//                    button_access_base();
                }

                // wire tunnel
                rotate([90, 0, 0])
                rotate([0, 0, 45])
                translate([0, 0, 7])
                rcube(6, 6, 60, 1);
            }
        }
        
    }
}

intersection()
{
    ccube(100, 100, 10);
    button_test_base(0, 0, 1);
}

module key_plug_base()
{
    // 3mm cut around the hole for seating/overhang and keycap
    rcube(20, 20, 20, 1);
    // I accidentally modelled this backwards, but it's actually simpler that way.
    // Just mirror it left-right here.
    scale([-1, 1, 1])
    rotate([0, 0, 180])
    translate([-7, -7, -8])
    union()
    {
        translate([0.5, 0.5])
        union()
        {
            // plug shape
            linear_extrude(height=3)
            difference()
            {
                intersection()
                {
                    offset(2)
                    offset(-2)
                    square([13, 8.25]);
                    square([11, 6.25]);
                }
                translate([4, 4])
                offset(2)
                offset(-2)
                square([10, 10]);
            }
            
            translate([0, 0, -7])
            linear_extrude(height=10)
            {
                // clearance for wire and installing from the bottom
                translate([-3, 3.25])
                square([3, 10.75]);
                translate([11, 0])
                square([3, 14]);
                translate([0, 10])
                square([14, 4]);
            }
        }
            
        // top part of hole
        translate([0, 0, 3])
        {
            hull()
            {
                translate([-2.5, 0, 0])
                linear_extrude(height=2)
                square([17, 14.5]);
                
                translate([0, 0, 4])
                linear_extrude(height=0.01)
                square([14, 14]);
            }
            translate([0, 0, 4])
            linear_extrude(height=10)
            square([14, 14]);
        }
        
        // center peg
        linear_extrude(height=3)
        translate([7, 7])
        circle(d=4);

        // push-out hole for removing the plug if needed
        translate([7, 2.5, -7])
        linear_extrude(height=10)
        circle(d=2);
        
    }
}

//difference()
//{
//    color("red")
//    rcube(22, 22, -10, 2);
//
//    key_plug_base();
//}

