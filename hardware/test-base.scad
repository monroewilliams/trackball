use <trackball.scad>
// Test base for recessed button cutouts.

$fn=30;

module button_test_base(front_overhang, back_overhang)
{
    difference()
    {
        border = 2;

        front_extra = ((front_overhang > 0)?(front_overhang + 3):(3));
        back_extra = ((back_overhang > 0)?(back_overhang + 1):(3));
        length = border + front_extra + 14 + back_extra + border;
        offset_adjustment = (front_extra - back_extra) / 2;

        // main body
        translate([0, offset_adjustment, 0])
        rcube(border + 20 + border, length, 16, 2);
        
        // wire tunnel
        rotate([-90, 0, 0])
        rotate([0, 0, 45])
        rcube(6, 6, 60, 1);
        
        translate([0, 0, 10])
        {
            // switch cutout
            button_cutout(front_overhang, back_overhang);
            shadow_hull()
            button_access_base();
        }
        
    }
}

button_test_base(5, 20);
