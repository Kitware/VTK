catch {load vtktcl}
#
# This is a regression test script for VTK.
#

#
# if VTK_RESULTING_IMAGE_PATH is defined, then use if to qualify the error 
# and test images
#
if { [catch {set VTK_RESULTING_IMAGE_PATH $env(VTK_RESULTING_IMAGE_PATH)/}] != 0} { set VTK_RESULTING_IMAGE_PATH "" }

# on windows shut off global warnings because they hold up the tests
if {$tcl_platform(os) == "Windows NT"} {
    vtkObject rtTempObject;
    rtTempObject GlobalWarningDisplayOff;
}

#
# if VTK_PLATFORM is defined, then use if to get another valid image
#
if { [catch {set VTK_PLATFORM ".$env(VTK_PLATFORM)"}] != 0} { set VTK_PLATFORM "" }

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

# set up the log file descriptor
if { [catch {set logFileName $env(VTK_REGRESSION_LOG) }] != 0} {
    set logFile stdout
} else {
    set logFile [open $logFileName "a+"]
}

# first find all the examples. they can be defined on command line or in
# current directory
if { $argv != ""} {
    set files $argv
} else {
    set files [lsort [glob {[A-z]*.tcl}]]
}

# remove files that are not suitable for regression tests or simply don't 
# work right now
set noTest {
    rt.tcl rtAll.tcl rib.tcl TkInteractor.tcl TkRenderWidget.tcl 
    RenderWidget.tcl 
    rtExamples.tcl polyViewer.tcl KeyFrame.tcl cameraKey.tcl timing.tcl 
    Decimate.tcl assembly2.tcl connPineRoot.tcl deciHawa.tcl 
    deciPineRoot.tcl deleted.tcl mcTest.tcl viewMCubesFile.tcl
    volTkInteractor.tcl spikeColor.tcl tkwin.tcl 3dsToRIB.tcl backdrop.tcl
    ShotNoiseInclude.tcl TestFeatureAnd.tcl TestHistogram.tcl 
    TestHistogramEqualization.tcl TestMIPFilter.tcl
    TestSubsample3D.tcl
    TestWriter.tcl Timing.tcl WindowLevelInterface.tcl
    vtkImageInclude.tcl}

for {set i 0} {$i < [llength $noTest]} {incr i} {
    if {[set pos [lsearch $files [lindex $noTest $i]]] != -1} {
      set files [lreplace $files $pos $pos ]
    }
}



# now do the tests
foreach afile $files {
    #
    # only tcl scripts with valid/ images are tested
    set validImage $validPath/${afile}${VTK_PLATFORM}.ppm
    
    #
    # first see if there is an alternate image for this architecture
    if {[catch {set channel [open ${validImage}]}] != 0 } {
	set validImage $validPath/$afile.ppm
	if {[catch {set channel [open ${validImage}]}] != 0 } {
	    puts $logFile "WARNING: There is no valid image for $afile"
	    continue
	} else {
	    close $channel
	}
    } else {
	close $channel
    }
    vtkMath rtExMath
    rtExMath RandomSeed 6
    
    puts -nonewline $logFile "$afile took "
    flush stdout
    puts -nonewline $logFile "[expr [lindex [time {source $afile} 1] 0] / 1000000.0] seconds "
    
    vtkWindowToImageFilter w2if
    
    # look for a renderWindow ImageWindow or ImageViewer
    # first check for some common names
    if {[info commands renWin] == "renWin"} {
	w2if SetInput renWin
	set threshold 10
    } else {
	set threshold 0
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
    
    if {[catch {set channel [open ${validImage}]}] != 0 } {
	puts $logFile "\nWARNING: Creating a valid image for $afile"
	vtkPNMWriter rtpnmw
	rtpnmw SetInput [w2if GetOutput]
	rtpnmw SetFileName ${validImage}
	rtpnmw Write
	vtkCommand DeleteAllObjects
	catch {destroy .top}
	continue
    }
    close $channel
   
    vtkPNMReader rtpnm
    rtpnm SetFileName ${validImage}
   
    vtkImageDifference imgDiff
    
    if {$threshold == 0} {imgDiff AllowShiftOff; imgDiff SetThreshold 1}
    imgDiff SetInput [w2if GetOutput]
    imgDiff SetImage [rtpnm GetOutput]
    imgDiff Update

    # a test has to be off by at least threshold pixels for us to care   
    if {[imgDiff GetThresholdedError] <= $threshold} {
	puts $logFile "and Passed"
    } else {
	puts $logFile "but failed with an error of [imgDiff GetThresholdedError]"
	vtkPNMWriter rtpnmw
	rtpnmw SetInput [imgDiff GetOutput]
	rtpnmw SetFileName "${VTK_RESULTING_IMAGE_PATH}$afile.error.ppm"
	rtpnmw Write
	vtkPNMWriter rtpnmw2
	rtpnmw2 SetInput [w2if GetOutput]
	rtpnmw2 SetFileName "${VTK_RESULTING_IMAGE_PATH}$afile.test.ppm"
	rtpnmw2 Write
    }
    
    vtkCommand DeleteAllObjects
    catch {destroy .top}
}

exit