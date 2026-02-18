$fa = 1;
$fs = 0.01;

function bezier_point(c1, c2, c3, c4, pct) =
    c1 * pow(1 - pct, 3)
  + c2 * pow(1 - pct, 2) * pct * 3
  + c3 * pow(pct,     2) * (1 - pct) * 3
  + c4 * pow(pct,     3);

function bezier(p1, p2, p3, p4, steps = 50) =
  [for(step = [0 : steps])
     [bezier_point(p1[0], p2[0], p3[0], p4[0], step / steps),
      bezier_point(p1[1], p2[1], p3[1], p4[1], step / steps)]];

module convex_fillet(size = 4, rotation = 45, x = 1, practice = false) {
    difference() {
        if (!practice) {
            rotate([0, 0, rotation])           
            translate([size, size])
            circle(size * x);
        }
        circle(size);
    }
}

lightrail_depth = 3;
stud_height = 1;


module wedge() {
    plate_width = 3.5;
    plate_lip_width = 0.5;
    wedge_height = plate_width / 2 + lightrail_depth + stud_height - plate_lip_width;
    board_chopout_z = 2.0;
    battery_connector_slot_x = 9.2;
    wires_slot_x = 42;
    wires_slot_width = 6;
    
    c1 = [2.5, 15.5];
    c2 = [11, 19.5];
      
    c3 = [22.5, 20.4];
    c4 = [28, 15];

    c5 = [39, 10.5];
    c6 = [50, 4];

    difference() {
        linear_extrude(wedge_height)
        difference() {
            // wedge
            offset(r=0.5) {
                scale([0.94,0.94])
                translate([0.5,0.5])
                polygon(concat(
                    bezier([0, 0], c1, c2, [15.2, 19.9]),
                    bezier([15.2, 19.9], c3, c4, [29, 15]),
               //     [[30, 40], [29, 40]],
                    bezier([29, 15], c5, c6, [64, 0])
                ));
            }
            
            // coupling slot
            translate([20.6, 5.5])
            circle(1.5);
        
            // battery slot
            translate([10, 12.8]) // ->9.5
            rotate(-16.5)         // ->16
            offset(r=1) 
            square([28.5, 3.15]);
            
            // wires slot
            translate([wires_slot_x, 2.65])
            offset(r=0.5)
            square([wires_slot_width, 1]);
            
            // battery connector slot
            translate([battery_connector_slot_x, 2.65])
            offset(r=0.5)
            square([7.5, 5.5]);
        }
        
        // board slot, which doesn't go all the way through
        translate([2.2, 1.5, board_chopout_z])
        linear_extrude(wedge_height)
        offset(r=0.5)
        square([51, 0.65]);
        // little chip next to battery connector slot
        translate([battery_connector_slot_x - 2, 2.65, board_chopout_z])
        linear_extrude(wedge_height)
        offset(r=0.5)
        square([1.5, 0.5]);
        // slim chip midway along board on battery connector side
        translate([27, 2.65, board_chopout_z])
        linear_extrude(wedge_height)
        offset(r=0.5)
        square([5, 0.2]);
        
        // wires slot fillets
        translate([wires_slot_x - 1, 3.15, board_chopout_z])
        linear_extrude(wedge_height)
        convex_fillet(size=0.5, rotation=-90, practice=false);
        translate([wires_slot_x - 1 + wires_slot_width + 2, 3.15, board_chopout_z])
        linear_extrude(wedge_height)
        convex_fillet(size=0.5, rotation=180, practice=false);
        
        // slim chip midway fillets
        translate([27 - 1 + 0.21, 2.95, board_chopout_z])
        linear_extrude(wedge_height)
        convex_fillet(size=0.3, rotation=-90, practice=false);
        translate([27 - 1 + 5 + 1.79, 2.95, board_chopout_z])
        linear_extrude(wedge_height)
        convex_fillet(size=0.3, rotation=180, practice=false);
        
        // battery connector slot fillets
        translate([battery_connector_slot_x - 1 - 2, 3.15, board_chopout_z])
        linear_extrude(wedge_height)
        convex_fillet(size=0.5, rotation=-90, practice=false);
        translate([battery_connector_slot_x - 1, 4.15, board_chopout_z])
        linear_extrude(wedge_height)
        convex_fillet(size=0.5, rotation=-90, practice=false);
        translate([battery_connector_slot_x - 1 + 7.5 + 2, 3.15, board_chopout_z])
        linear_extrude(wedge_height)
        convex_fillet(size=0.5, rotation=180, practice=false);
    }
    

    
    /*
    // guide
    scale([0.93,0.99])
    color("green")
    polygon([
    [0.000000, 0.000000],
    [0.419167, 2.599841],
    [1.114298, 5.346851],
    [2.282457, 8.395321],
    [3.505490, 10.982801],
    [5.294308, 13.830581],
    [6.982213, 15.709241],
    [8.842473, 17.241301],
    [10.646021, 18.374301],
    [12.249896, 19.098791],
    [14.122354, 19.728381],
    [15.908463, 19.937661],
    [17.411060, 19.867961],
    [18.817353, 19.800761],
    [20.575728, 19.421711],
    [22.178324, 19.107821],
    [23.695343, 18.604931],
    [25.446541, 17.908611],
    [26.994934, 16.960051],
    [28.712189, 15.922771],
    [30.167887, 15.014881],
    [30.469269, 16.555531],
    [32.033596, 15.689661],
    [34.540286, 14.242001],
    [37.740775, 12.416761],
    [39.503697, 11.461541],
    [41.191395, 10.642451],
    [43.805775, 9.466061],
    [46.865615, 8.136241],
    [48.820890, 7.304751],
    [50.898655, 6.525211],
    [53.673458, 5.520921],
    [56.977601, 4.350881],
    [59.934605, 3.383591],
    [62.921304, 2.490091],
    [66.708119, 1.541421],
    [66.615899, -0.011994]]);

    // bezier control points
      translate(c1) color("red") circle(0.3);
      translate(c2) color("red") circle(0.3);
      translate(c3) color("red") circle(0.3);
      translate(c4) color("red") circle(0.3);
      translate(c5) color("red") circle(0.3);
      translate(c6) color("red") circle(0.3);

    //*/
}


module light_rail() {
    lr_drop = 9.5;
    lr_height = 13;
    
    // drop bar
    linear_extrude(lightrail_depth) {
        translate([4.5, 0.4 - lr_drop])
        square([54, lr_drop]);
        
        // fillets
        difference() {
            translate([4, -0.51])
            square(0.5);
            translate([4.01, -0.52])
            circle(0.5);
        }
        difference() {
            translate([4, 0.49-lr_drop])
            square(0.5);
            translate([4.01, 0.99-lr_drop])
            circle(0.5);
        }
        difference() {
            translate([58.5, -0.51])
            square(0.5);
            translate([58.99, -0.52])
            circle(0.5);
        }
        difference() {
            translate([58.5, 0.49-lr_drop])
            square(0.5);
            translate([58.99, 0.99-lr_drop])
            circle(0.5);
        }
    }

    front_thickness = 0.5;
    led_strip_thickness = 2.1;

    //stilts
    for(position = [-7.96 : 20.79 : 95.99])
    translate([position, -lr_drop -lr_height/2, 0])
    linear_extrude(front_thickness + led_strip_thickness - 0.1)
    offset(0.5)
    square([0.79, 0.1]);
        
    // rail
    difference() {
        linear_extrude(lightrail_depth + stud_height)
        translate([-20, -lr_drop -lr_height])
        offset(r=0.5)
        square([128, lr_height]);

        // create a slot for the led strip to go in
        translate([-19, -(lr_drop + lr_height / 2 + 5.6), front_thickness])
        linear_extrude(led_strip_thickness)
        offset(r=0.5)
        square([126, 11.2]);
       
        // and a slot at the end to post the lights in and let wires out       
        translate([-21, -(lr_drop + lr_height / 2 + 5.5), 2.5])
        linear_extrude(10)
        offset(r=0.5)
        square([9, 11]);
        // with its fillets
        translate([-20.2, -(lr_drop + lr_height / 2 + 6.3), 2.5])
        linear_extrude(10)
        convex_fillet(size=0.3, rotation=90, practice=false);
        translate([-20.2, -(lr_drop + 0.2), 2.5])
        linear_extrude(10)
        convex_fillet(size=0.3, rotation=180, practice=false);    
        translate([-11.132, -(lr_drop + 0.505), 2.5])
        linear_extrude(10)
        convex_fillet(size=0.5, rotation=190, x = 0.7, practice=false);   
        translate([-11.132, -(lr_drop + lr_height - 0.505), 2.5])
        linear_extrude(10)
        convex_fillet(size=0.5, rotation=80, x = 0.7, practice=false);    
        
        // and some windows for the lights
        for(position = [-14.5 : 6.93 : 105])
        translate([position, -lr_drop -lr_height/2, -0.1])
        cylinder(h=1, r=2.5);
        
        // chop out some bits so we don't need to bridge too much
        for(position = [-26.96 : 20.79 : 76.99])
        translate([position, -lr_drop -lr_height + 1.5, 2.5])
        linear_extrude(10)
        offset(r=0.5)
        square([18, lr_height - 3]);
        translate([97.78, -lr_drop -lr_height + 1.5, 2.5])
        linear_extrude(10)
        offset(r=0.5)
        square([8.5, lr_height - 3]);
    }

}

module plug() {
    wedge();
    light_rail();
}

mirror([1, 0, 0])
plug();
  