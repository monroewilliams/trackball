use <trackball.scad>

$fa = 4;

// Useful cheats for testing new components
bottom = -33.5;
ball_diameter=57;
ball_radius=ball_diameter/2;

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
    difference()
    {
        // color("white", 0.75)
        intersection()
        {
            // Clip to above bottom surface
            // render()
            translate ([0, 0, bottom])
            ccube(400, 400, 400);

            union()
            {
                // left cut
                // render(4)
                body_left_cut();
                // tail cut
                // render(4)
                // body_tail_cut();
            }
                    
            // right cut
            // render(4)
            body_right_cut();

            // back cut
            // render(4)
            body_back_cut();
        
        }
    }

}

// This is the full assembly with all the cutouts. I find it useful when I just want to visualize
// certain of the cutouts on the full body.
// Uncommenting the render() calls on parts you're not actively modifying sometimes helps interactive performance.

if (true)
union()
{
    difference()
    {
        // test_body();

        // render(4) 
        body();
        // render(4) 
        // body_minimal();
        // render(6) 
        ball_cutout();
        // render() 
        bearing_cutouts();
        // render(4) 
        sensor_cutouts();
        // render(6) 
        button_cutouts();
        // render() 
        wire_cutouts();
    }

    // ball();
}


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


