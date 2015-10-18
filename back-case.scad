module back_case()
{
   union() {
      difference() {
         translate([0, 30, -1])
            cube([79, 119, 56], center = true);
         translate([0, 30, 2])
            cube([75, 115, 55], center = true);
         translate([0, 10, 10])
            cube([68, 59, 100], center = true);
         translate([30, 55, 15])
            cube([30, 3, 40], center = true);
      }
   }
}

back_case();

