catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

#
# This is a regression test script for VTK.
#

# determine where the "valid" images are kept
if { [catch {set validPath $env(VTK_VALID_IMAGE_PATH) }] != 0} {
    set validPath valid
} else {
    # are we in graphics or imaging or ?
    set fullPathList [file split [file dirname [pwd]]]
    set kitName [lindex $fullPathList [expr [llength $fullPathList] - 1 ]]
    
    # append the kitname onto the validPath
    set validPath "$validPath/$kitName"
}

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
   Decimate.tcl connPineRoot.tcl aniIso.tcl deciHawa.tcl 
   deciPineRoot.tcl deleted.tcl mcTest.tcl viewMCubesFile.tcl
   volTkInteractor.tcl spikeColor.tcl tkwin.tcl 
   purifyExamples.tcl 
   combColorIso.tcl
   stCone.tcl
   genPineRoot.tcl pineRoot.tcl hawaii.tcl
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
    set validImage $validPath/${afile}.tif
    if {[catch {set channel [open ${validImage}]}] != 0 } {
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
	 } else {
	     if {[info exists viewer]} {
		 w2if SetInput [$viewer GetImageWindow]
	     }
	 }
      }
   }

   # run the event loop quickly to map any tkwidget windows
   wm withdraw .
   update

   w2if Update
   vtkCommand DeleteAllObjects
    catch {destroy .top}
    catch {destroy .geo}
}

exit
