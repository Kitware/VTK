#
# This is a regression test script for VTK.
#

#vtkCommand DebugOn;

# first find all the examples
set files [lsort [glob {*.tcl}]];

# remove support files that we know are not examples
if {[set pos [lsearch $files "vtkInt.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "vtkInclude.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "colors.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "rtExamples.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
# remove files that are not appropriate or include random sources
# or just take way too long
if {[set pos [lsearch $files "Mace.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
# assembly 2 should be in there
if {[set pos [lsearch $files "assembly2.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "connPineRoot.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "cylMap.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "deciHawa.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "deciPineRoot.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "deleted.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "genPineRoot.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "mcTest.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "viewMCubesFile.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "tkwin.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "sphereMap.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}


# now do the tests
foreach afile $files {
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

   if {[imgDiff GetThresholdedError] == 0.0} {
       puts "Passed Test for $afile"
   } else {
       puts "Failed Test for $afile with an error of [imgDiff GetThresholdedError]"
   }
   
   vtkCommand DeleteAllObjects;
}

exit