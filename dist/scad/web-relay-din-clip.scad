$fn=50;

// Clip thickness
DIN_CLIP_W = 7;

module din_clip() {
    union() {
        difference() {
            cube([16,43,DIN_CLIP_W], center=true);
            hull() {
                translate([9,-18.8,0]) cylinder(d=1,h=8,center=true);
                translate([-3.7,-18.8,0]) cylinder(d=1,h=8,center=true);
            }
            translate([7.5,-1,0]) cube([4,36.3,8],center=true);
        }
        hull() {
            translate([11,-21,0]) cylinder(d=1,h=DIN_CLIP_W,center=true);
            translate([8,-21,0]) cylinder(d=1,h=DIN_CLIP_W,center=true);
            translate([7.2,-17.5,0]) cylinder(d=0.5,h=DIN_CLIP_W,center=true);
        }
        hull() {
            translate([8.5,20,0]) cylinder(d=3,h=DIN_CLIP_W,center=true);
            translate([7.8,20,0]) cylinder(d=3,h=DIN_CLIP_W,center=true);
            translate([8.9,14,0]) cylinder(d=2,h=DIN_CLIP_W,center=true);
        }
    }
}

module din_relay_board() {
    difference() {
        union() {
            din_clip();
            translate([-10,0,0]) cube([6,90,DIN_CLIP_W], center=true);
            translate([-11.5,-82.5/2,0]) cube([9,9,DIN_CLIP_W], center=true);
            translate([-11.5,+82.5/2,0]) cube([9,9,DIN_CLIP_W], center=true);
        }
        
        translate([-10,-82.5/2,0]) rotate([0,90,0]) cylinder(d=3.2,h=15,center=true);
        translate([-10,+82.5/2,0]) rotate([0,90,0]) cylinder(d=3.2,h=15,center=true);
        
        translate([-4,-82.5/2,0]) rotate([0,90,0]) cylinder(d=5.7,h=10,center=true, $fn=6);
        translate([-4,+82.5/2,0]) rotate([0,90,0]) cylinder(d=5.7,h=10,center=true, $fn=6);
    }
}

din_relay_board();

