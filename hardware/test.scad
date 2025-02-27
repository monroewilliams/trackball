// Using 'include' instead of 'use' here means we can use global definitions from trackball.scad, not just modules.
include <trackball-hollow.scad>

// Defines here can override global defines in trackball.scad, so you can change parameters like ball diameter
// and button positions here.
// ball_diameter=57;

//button_params = [
//    // example_button_params[0],
//    // example_button_params[1],
//    // example_button_params[2],
//    // example_button_params[3],
//    // example_button_params[4]
//];


// This renders the full trackball from trackball.scad.
// full();

// This is the full body shell, without any cutouts
// body();

// Individual pieces of the CSG can also be rendered, which is useful when tweaking them in isolation
// body_right_cut();
// body_left_cut();

// This is the contents of the body() module, reproduced here to make commenting out parts easier.
module test_body()
{
    // color("white", 0.75)
    intersection()
    {
        // Clip to above bottom surface
        translate ([0, 0, bottom])
        ccube(400, 400, 400);
        union()
        {
            intersection()
            {
                // left cut
                body_left_cut();
                // tail cut
                // body_tail_cut();
                        
                // right cut
                body_right_cut();

            }        
 
            // body_button_supports();

            // sensor_shells();
        }

    }
}


// For when you're just working on the underside cutouts...
module body_standin()
{
    translate([0, 0, bottom])
    ccube(80, 180, 50);
}

// Screw hole size test
//difference()
//{
//    cuboid([10, 10, 5]);
//    cyl(d = 2.75, h = 10);
//}

// Just the portion around the USB plug
if (true)
{
    intersection()
    {
        full_hollow(true);
        
        rotate([0, 0, 45])
        translate([0, 39, bottom])
        cuboid([50, 100, 12], anchor = FRONT+BOTTOM);
    }
}

// Third button hole + one bearing hole
if (false)
{
    intersection()
    {
        full_hollow();
        difference()
        {
            translate([-6, -9, bottom + 12])
            cuboid([27, 45, 50], anchor = [1, 1, -1]);

            cuboid([30, 16, 30], anchor = [1, 1, -1]);
        }
    }
}
    
// Just bearing test
if (false)
{
    intersection()
    {
        full();
        rotate([0, 0, 30])
        translate([-25, 0, bottom + 18])
        cuboid([8, 10, 10], anchor = CTR);
    }
}       



// This is the full assembly with all the cutouts. I find it useful when I just want to visualize
// certain of the cutouts on the full body.

if (false)
union()
{
    difference()
    {
        // alternates to the standard body
        // body();
        body_minimal();
        // body_standin();

        // body();
        ball_cutout();
        bearing_cutouts();
        sensor_cutouts();
        sensor_access_cutouts();
        // button_cutouts();
        wire_cutouts();
    }

    // ball();
}


// difference()
// {
//     sensor_shells();
//     sensor_cutouts();
//     sensor_access_cutouts();
// }


// color("blue", 0.25) 
// body();
// test_body();

// Rendering semi-transparent cut objects can be useful while tweaking.
// This only seems to work if they come _after_ non-transperent objects (othewise the colors don't take)
// color("blue", 0.25) 
// button_cutouts();
// body_back_cut();

// color("blue", 0.25)
// translate([0, 0, 100])
// linear_extrude(height=1)
// body_outline();

// color("blue", 0.75)
// body_right_cut();


