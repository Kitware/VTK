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

# remove files that are not suitable for regression tests or simply don't work right now
set noTest {
   rt.tcl rtAll.tcl rib.tcl TkInteractor.tcl TkRenderWidget.tcl RenderWidget.tcl 
   rtExamples.tcl polyViewer.tcl KeyFrame.tcl cameraKey.tcl timing.tcl 
   Decimate.tcl assembly2.tcl connPineRoot.tcl aniIso.tcl deciHawa.tcl 
   deciPineRoot.tcl deleted.tcl mcTest.tcl viewMCubesFile.tcl vol.tcl
   volTkInteractor.tcl spikeColor.tcl tkwin.tcl 3dsToRIB.tcl
   purifyExamples.tcl 

   genHead.tcl genPineRoot.tcl headBone.tcl pineRoot.tcl hawaii.tcl
   mcubes.tcl LOx.tcl VolCutKnee.tcl recursiveDC.tcl

}

for {set i 0} {$i < [llength $noTest]} {incr i} {
   if {[set pos [lsearch $files [lindex $noTest $i]]] != -1} {
      set files [lreplace $files $pos $pos ]
   }
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

   vtkMultiThreader rtMultiThreader
   rtMultiThreader SetGlobalMaximumNumberOfThreads 1 
  
   puts -nonewline "$afile took "
   puts "[expr [lindex [time {source $afile} 1] 0] / 1000000.0] seconds "

   vtkWindowToImageFilter w2if
   # look for a renderWindow ImageWindow or ImageViewer
   # first check for some common names
   if {[info commands renWin] == "renWin"} {
      w2if SetInput renWin
   } else {
      if {[info commands viewer] == "viewer"} {
	 w2if SetInput [viewer GetImageWindow]
	 viewer Render
      } else {
	 if {[info commands imgWin] == "imgWin"} {
	    w2if SetInput imgWin
	    imgWin Render
	 }
      }
   }

   # run the event loop quickly to map any tkwidget windows
   wm withdraw .
   update

   w2if Update
   vtkCommand DeleteAllObjects
}

exit
