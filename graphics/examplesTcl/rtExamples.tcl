catch {load vtktcl}
#
# This is a regression test script for VTK.
#

#vtkCommand DebugOn

# first find all the examples
set files [lsort [glob {[a-z]*.tcl}]]

# remove support files that we know are not examples
if {[set pos [lsearch $files "TkInteractor.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "TkRenderWidget.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "RenderWidget.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "rtExamples.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "polyViewer.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
# remove files that are not appropriate or include random sources
# or just take way too long
# assembly 2 should be in there
if {[set pos [lsearch $files "Decimate.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "assembly2.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "connPineRoot.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "aniIso.tcl"]] != -1} {
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
if {[set pos [lsearch $files "mcTest.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "viewMCubesFile.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "vol.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "volTkInteractor.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "spikeColor.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "streamV.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "tkwin.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "sphereMap.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "motor.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}


# now do the tests
foreach afile $files {
   source "$afile"

   vtkRendererSource renSrc
   renSrc WholeWindowOn
   renSrc SetInput ren1
   vtkPNMReader pnm
   pnm SetFileName "valid/$afile.ppm"
   
   vtkImageDifference imgDiff
   imgDiff SetInput [renSrc GetOutput]
   imgDiff SetImage [pnm GetOutput]
   imgDiff Update

# a test has to be off by at least ten pixels for us to care   
   if {[imgDiff GetThresholdedError] < 10.0} {
       puts "Passed Test for $afile"
   } else {
       puts "Failed Test for $afile with an error of [imgDiff GetThresholdedError]"
         vtkPNMWriter pnmw
         pnmw SetInput [imgDiff GetOutput]
         pnmw SetFileName "$afile.error.ppm"
         pnmw Write
         vtkPNMWriter pnmw2
         pnmw2 SetInput [renSrc GetOutput]
         pnmw2 SetFileName "$afile.test.ppm"
         pnmw2 Write
   }
   
   vtkCommand DeleteAllObjects
}

