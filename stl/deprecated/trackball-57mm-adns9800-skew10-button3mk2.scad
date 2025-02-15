include <../hardware/trackball.scad>

ball_diameter=57;

sensor_skew_angle=10;

sensor_params = [
    [180, true, sensor_type_adns9800],
    [45, true, sensor_type_adns9800]
];

// button_params = [
//     example_button_params[0],
//     example_button_params[1],
//     example_button_params[2]
// ];

full();