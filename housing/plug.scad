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




c1 = [2.0, 15.3];
c2 = [14.5, 21];
  
c3 = [20, 22];
c4 = [50, 5];

linear_extrude(45)
difference() {
    // wedge
    offset(r=0.5) {
        scale([0.94,0.94,0.94])
        translate([0.5,0.5])
        polygon(concat(
            bezier([0, 0], c1, c2, [15, 21]),
            bezier([15, 21], c3, c4, [60, 0])
        ));
    }
    
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
}





/*
// bezier control points
  translate(c1) color("red") circle(0.3);
  translate(c2) color("red") circle(0.3);
  translate(c3) color("red") circle(0.3);
  translate(c4) color("red") circle(0.3);

// guidelines
  translate([05, 13]) color("black") circle(0.6);
  translate([10, 18]) color("black") circle(0.6);
  translate([15, 21]) color("black") circle(0.6);
  translate([20, 20]) color("black") circle(0.6);
  translate([25, 17.5]) color("black") circle(0.6);
  translate([30, 16]) color("black") circle(0.6);
  translate([35, 14]) color("black") circle(0.6);
  translate([40, 12]) color("black") circle(0.6);
  translate([45, 8]) color("black") circle(0.6);
//*/
  