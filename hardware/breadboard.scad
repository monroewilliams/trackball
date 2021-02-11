use <trackball.scad>
// Breadboard cutout test

difference()
{
    // Uncomment this to make a hollow shell with the breadboard cutout inside
    // minkowski()
    // {
    //     breadboard_cutout();
    //     intersection()
    //     {
    //         $fn = 15;
    //         sphere(r=1);
    //         ccube(10, 10, 10);
    //     }
    // }

    breadboard_cutout();
}
