module front_case()
{
   module front() {
      union() {
         difference() {
            translate([0, 30, -4.25])
               cube([74, 114, 8], center = true);
            translate([0, 30, -5.75])
               cube([70, 110, 6], center = true);
         }
         translate([0, 50, -5]) union() {
            translate([-20.5, -19.5, 0])
               cylinder(h = 5, r1 = 0.75, r2 = 0.75, center = true);
            translate([-20.5, 19.5, 0])
               cylinder(h = 5, r1 = 0.75, r2 = 0.75, center = true);
            translate([20.5, -19.5, 0])
               cylinder(h = 5, r1 = 0.75, r2 = 0.75, center = true);
            translate([20.5, 19.5, 0])
               cylinder(h = 5, r1 = 0.75, r2 = 0.75, center = true);
         }
         intersection() {
            sphere(r = 13, center = true);
            translate([0, 0, 0.75 * 26/8])
               cube([26, 26, 26/4], center = true);
         }
         translate([0, 0, -5]) union() {
            translate([-11.5, 8.5, 0])
               cylinder(h = 5, r1 = 1, r2 = 1, center = true);
            translate([-11.5, -11, 0])
               cylinder(h = 5, r1 = 1, r2 = 1, center = true);
            translate([11.5, 8.5, 0])
               cylinder(h = 5, r1 = 1, r2 = 1, center = true);
            translate([11.5, -11, 0])
               cylinder(h = 5, r1 = 1, r2 = 1, center = true);
         }
      }
   }

   module lcd() {
      cube([41, 35, 5], center = true);
   }

   module joystick() {
      union() {
         cube([17, 16, 12], center = true);
         translate([10, 0, 0])
            cube([4, 10.5, 12], center = true);
         translate([0, 9.5, 0])
            cube([10.5, 4, 12], center = true);
         translate([0, -11, -3])
            cube([10.5, 7.5, 6], center = true);
         translate([-9.5, 0, -3])
            cube([3, 8.5, 6], center = true);
         translate([0, -9.5, -1])
            cube([3.5, 5, 10], center = true);
      }
   }

   module components() {
      union() {
         translate([0, 50, -2])
            lcd();
         joystick();
      }
   }

   difference() {
      front();
      components();
   }
/*
		translate([0, 0, 5])
			cylinder(h = 50, r1 = 20, r2 = 5, center = true);
*/
}

front_case();

