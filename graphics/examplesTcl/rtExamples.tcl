catch {load vtktcl}
#
# This is a regression test script for VTK.
#

# first find all the examples. they can be defined on command line or in
# current directory

if { $argv != ""} {
  set files $argv
  } else {
  set files [lsort [glob {[A-z]*.tcl}]]
}

# remove support files that we know are not examples
if {[set pos [lsearch $files "rt.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "rtAll.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "rib.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
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
if {[set pos [lsearch $files "KeyFrame.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "cameraKey.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "timing.tcl"]] != -1} {
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
if {[set pos [lsearch $files "tkwin.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}
if {[set pos [lsearch $files "3dsToRIB.tcl"]] != -1} {
   set files [lreplace $files $pos $pos ]
}

# now do the tests
foreach afile $files {
    #
    # only tcl scripts with valid/ images are tested
    if {[catch {set channel [open "valid/$afile.ppm"]}] != 0 } {
     puts "WARNING: There is no valid image for $afile"
     continue
    }
   close $channel

   vtkMath rtExMath
   rtExMath RandomSeed 6

   puts -nonewline "$afile took "
   puts -nonewline "[expr [lindex [time {source $afile} 1] 0] / 1000000.0] seconds "

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
       puts "and Passed"
   } else {
       puts "but failed with an error of [imgDiff GetThresholdedError]"
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

exit