#
# This is a regression test script for VTK.
#

#vtkCommand DebugOn;

# first find all the examples
set files [lsort [glob {*.tcl}]];

# remove support files that we know are not examples
if {[set pos [lsearch $files "vtkInt.tcl"]] != -1} {
   set files [lreplace $files $pos [expr $pos + 1] ]
}
if {[set pos [lsearch $files "vtkInclude.tcl"]] != -1} {
   set files [lreplace $files $pos [expr $pos + 1] ]
}
if {[set pos [lsearch $files "colors.tcl"]] != -1} {
   set files [lreplace $files $pos [expr $pos + 1] ]
}
if {[set pos [lsearch $files "rtExamples.tcl"]] != -1} {
   set files [lreplace $files $pos [expr $pos + 1] ]
}
if {[set pos [lsearch $files "Mace.tcl"]] != -1} {
   set files [lreplace $files $pos [expr $pos + 1] ]
}
if {[set pos [lsearch $files "assembly2.tcl"]] != -1} {
   set files [lreplace $files $pos [expr $pos + 1] ]
}

# now do the tests
foreach afile $files {
   puts "Testing file $afile";
   source "$afile";

   vtkRendererSource renSrc;
   renSrc WholeWindowOn;
   renSrc SetInput $ren1;
   vtkPNMReader pnm;
   pnm SetFilename "valid/$afile.ppm";
   
   vtkImageDifference imgDiff;
   imgDiff SetInput [renSrc GetOutput];
   imgDiff SetImage [pnm GetOutput];
   imgDiff Update;

   if {[imgDiff GetError] == 0.0} {
      lappend summary "Passed Test for $afile\n"
      puts "Passed Test for $afile"
   } else {
      lappend summary "Failed Test for $afile\n"
      puts "Failed Test for $afile"
   }
   
   vtkCommand DeleteAllObjects;
}

puts $summary