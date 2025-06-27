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

module fillet() {
    difference() {
        square(1);
       // translate([x, x]) circle(x/2);
        translate([0.75, 0.25]) circle(0.5);
       // translate([0, x]) circle(x/2);
       // translate([0, 0]) circle(x/2);
    }
}




c1 = [2.5, 15.5];
c2 = [11, 19.5];
  
c3 = [22.5, 20.4];
c4 = [28, 15];

c5 = [39, 10.5];
c6 = [50, 4];

plate_width = 21;
plate_lip_width = 0.5;
lightrail_depth = 4;
stud_height = 9;

wedge_height = plate_width / 2 + lightrail_depth + stud_height - plate_lip_width;

//mirror()
{
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
    
    // wire slot
    translate([11.5, 14])
    circle(2.5);
    
    // peg slot 1
    translate([48.5, 2.5])
    circle(1.3);
    
    // peg slot 2
    translate([31.2, 3.2])
    circle(1.3);
    
    // battery slot
    translate([18, 13.5])
    rotate(-24)
    offset(r=1) 
    square([28, 3]);
    
    // board slot
    translate([5, 2.5]) 
    offset(r=1)
    square([22, 6]);
    
    // fillet #1
    difference() {
        translate([27.75, 7.54])
        rotate(-12)
        square(0.6);
        translate([28.3, 7.49])
        circle(0.301);
    }
    
    // fillet #2
    difference() {
        translate([23.1, 9.45])
        rotate(-12)
        square([1.5, 0.7]);
        translate([23.1, 9.80])
        circle(0.305);
    }
    //*/
}

// light rail (lr)
lr_drop = 3.5;
lr_height = 14;
lightrail = [[-20, -lr_drop],
             [-20, -(lr_drop + lr_height)],
             [110, -(lr_drop + lr_height)],
             [110, -lr_drop]];

linear_extrude(lightrail_depth) {
    offset(r=0.5)
    polygon(concat([[0.47, 0.4], [0.47, -lr_drop]],
                   , lightrail,
                   [[60.63, -lr_drop], [60.63, 0.4]]));

    // fillets
//    difference() {
//        translate([4, -0.51])
//        square(0.5);
//        translate([4.01, -0.52])
//        circle(0.5);
//    }
    difference() {
        translate([-0.53, 0.49-lr_drop])
        square(0.5);
        translate([-0.52, 0.99-lr_drop])
        circle(0.5);
    }
//    difference() {
//        translate([58.5, -0.51])
//        square(0.5);
//        translate([58.99, -0.52])
//        circle(0.5);
//    }
    difference() {
        translate([61.13, 0.49-lr_drop])
        square(0.5);
        translate([61.62, 0.99-lr_drop])
        circle(0.5);
    }
}


stud_inset_top = 3;
translate([-5, -(lr_drop + 5)])
cylinder(lightrail_depth + stud_height, stud_inset_top, 3);
translate([45, -(lr_drop + 5)])
cylinder(lightrail_depth + stud_height, stud_inset_top, 3);
translate([95, -(lr_drop + 5)])
cylinder(lightrail_depth + stud_height, stud_inset_top, 3);
//linear_extrude(8)
//offset(r=0.5)
//polygon(lightrail);
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
  