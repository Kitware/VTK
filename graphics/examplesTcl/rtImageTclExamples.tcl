catch {load vtktcl}
#
# This is a regression test script for VTK.
#
# Script returns status depending on outcome
#    returnStatus = 0 (success)
#    returnStatus = 1 (test failed)
#    returnStatus = 2 (no valid image)
#    returnStatus = 3 (excluded test)
# if multiple tests are run, the return status of the last test is returned
#
# The following environment variables can be used to control the
# behavior of this script:
#    VTK_RESULTS_PATH -     where to put regression images, defaults to ./
#    VTK_PLATFORM -         the OS of the computer, defaults to ""
#    VTK_VALID_IMAGE_PATH - where the valid images are, defaults to ./valid
#    VTK_REGRESSION_LOG -   where to send log messages, defaults to stdout
#    VTK_REGRESSION_XML -   where to put xml output, defaults to rtLog.xml
#    VTK_ROOT -             vtk root directory, defaults to ../../../

set returnStatus 3

#
# if VTK_RESULTS_PATH is defined, then use if to qualify the error 
# and test images
#
if { [catch {set VTK_RESULTS_PATH $env(VTK_RESULTS_PATH)/}] != 0} { set VTK_RESULTS_PATH "" }

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

# set up the xml file descriptor
if { [catch {set xmlFileName $env(VTK_REGRESSION_XML) }] != 0} {
    set xmlFileName rtLog.xml
}

# first find all the examples. they can be defined on command line or in
# current directory
if { $argv != ""} {
    set files $argv
} else {
    set files [lsort [glob {*.tcl}]]
}

# remove files that are not suitable for regression tests or simply don't 
# work right now
set noTest {
    rt.tcl rtAll.tcl rib.tcl TkInteractor.tcl TkRenderWidget.tcl 
    RenderWidget.tcl purifyExamples.tcl
    mccases.tcl mccasesui.tcl InteractorDemo.tcl
    rtExamples.tcl polyViewer.tcl KeyFrame.tcl cameraKey.tcl timing.tcl 
    Decimate.tcl connPineRoot.tcl deciHawa.tcl 
    deciPineRoot.tcl deleted.tcl viewMCubesFile.tcl
    volTkInteractor.tcl spikeColor.tcl tkwin.tcl backdrop.tcl
    ShotNoiseInclude.tcl 
    TkImageViewerInteractor.tcl TkViewer.tcl TkViewer2.tcl
    Timing.tcl WindowLevelInterface.tcl
    stCone.tcl
    vtkHistogramWidget.tcl
    vtkImageInclude.tcl rtProcessCPUTimes.tcl CPUTimeTable.tcl}

for {set i 0} {$i < [llength $noTest]} {incr i} {
    if {[set pos [lsearch $files [lindex $noTest $i]]] != -1} {
      set files [lreplace $files $pos $pos ]
    }
}

proc decipadString { str before total } {
    set x [string first "." $str]
    if { $x == -1 } { 
	set str "${str}.0"
    }

    set x [string first "." $str]
    while { $x >= 0 && $x < $before } {
	set str " $str"
	set x [string first "." $str]
    }

    if { [string length $str] >= $total } {
        return [string range $str 0 [expr $total - 1]]
    }

    while { [string length $str] < $total } {
        set str "${str}0"
    }
    return $str
}

# Convenience script to pad a string out to a given length
proc padString { str amount } {
    while { [string length $str] < $amount } {
        set str " $str"
    }
    return $str
}


# if VTK_ROOT is defined, then use it to find the CPU processing scripts
if { [catch {set VTK_ROOT $env(VTK_ROOT)}] != 0} { set VTK_ROOT "../../../" }
if { [catch {set VTK_DIR $env(VTK_DIR)}] != 0} { set VTK_DIR "vtk" }
source $VTK_ROOT/$VTK_DIR/graphics/examplesTcl/rtProcessCPUTimes.tcl
ReadCPUTimeTable

# now do the tests
foreach afile $files {
    #
    # only tcl scripts with valid/ images are tested
    set validImage $validPath/${afile}${VTK_PLATFORM}.tif
    
    #
    # first see if vtk has been built --with-patented
    if { [info command vtkMarchingCubes] == "" } {
	set validImage $validPath/$afile${VTK_PLATFORM}.withoutpatented.tif
	if {[catch {set channel [open ${validImage}]}] != 0 } {
            set validImage $validPath/$afile.withoutpatented.tif
            if {[catch {set channel [open ${validImage}]}] != 0 } {
                set validImage $validPath/$afile${VTK_PLATFORM}.tif
            } else {
                close $channel
            }
        } else {
            close $channel
        }
    }

    # Capture warnings and errors
    vtkXMLFileOutputWindow rtLog
      rtLog SetFileName $xmlFileName
      rtLog AppendOn
      rtLog FlushOn
      rtLog SetInstance rtLog

    #
    # now see if there is an alternate image for this architecture
    if {[catch {set channel [open ${validImage}]}] != 0 } {
	set validImage $validPath/$afile.tif
	if {[catch {set channel [open ${validImage}]}] != 0 } {
	    rtLog DisplayWarningText "There is no valid image for $afile"
            set returnStatus 2
	    vtkCommand DeleteAllObjects
	    continue
	} else {
	    close $channel
	}
    } else {
	close $channel
    }
    vtkMath rtExMath
      rtExMath RandomSeed 6
    
    # Start by putting the name out - right justify it and pad to 30 characters.
    # This line MUST start with a space so that name conflicts won't occur in
    # grep statements in other files
    set Name [padString $afile 29]
    puts -nonewline $logFile "\n $Name - "
    flush stdout
    
    vtkWindowToImageFilter w2if

    rtLog DisplayTag "<TestRun Name=\"$afile\">"
    rtLog DisplayTag "<StartDateTime>[clock format [clock seconds]]</StartDateTime>"

    # Create a timer so that we can get CPU time.
    # Use the tcl time command to get wall time
    vtkTimerLog timer
    set startCPU [timer GetCPUTime]
    set wallTime [decipadString [expr [lindex [time {catch {source $afile; if {[info commands iren] == "iren"} {renWin Render}}} 1] 0] / 1000000.0] 4 9]
    set endCPU [timer GetCPUTime]
    set CPUTime [decipadString [expr $endCPU - $startCPU] 3 8]
    puts -nonewline $logFile "$wallTime wall, $CPUTime cpu, "
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
    
    if {[catch {set channel [open ${validImage}]}] != 0 } {
	puts $logFile "\nWARNING: Creating a valid image for $afile"
        set returnStatus 2
	vtkTIFFWriter rttiffw
	rttiffw SetInput [w2if GetOutput]
	rttiffw SetFileName ${validImage}
	rttiffw Write
	vtkCommand DeleteAllObjects
	catch {destroy .top}
	continue
    }
    close $channel
   
    vtkTIFFReader rttiff
    rttiff SetFileName ${validImage}
   
    vtkImageDifference imgDiff
    
    if {$threshold == 0} {imgDiff AllowShiftOff; imgDiff SetThreshold 1}
    imgDiff SetInput [w2if GetOutput]
    imgDiff SetImage [rttiff GetOutput]
    imgDiff Update

    set imageError [decipadString [imgDiff GetThresholdedError] 4 9]

    rtLog DisplayTag "<Measurement Gauge=\"WallTime\"><Value>$wallTime</Value></Measurement>"
    rtLog DisplayTag "<Measurement Gauge=\"CPUTime\"><Value>$CPUTime</Value></Measurement>"
    rtLog DisplayTag "<Measurement Gauge=\"ImageDiff\"><Value>[imgDiff GetThresholdedError]</Value></Measurement>"
    # a test has to be off by at least threshold pixels for us to care   
    if {[imgDiff GetThresholdedError] <= $threshold} {
	set imageStatus "Passed"
        set returnStatus 0
    } else {
	set imageStatus "Failed"
        set returnStatus 1
	vtkTIFFWriter rttiffw
	rttiffw SetInput [imgDiff GetOutput]
	rttiffw SetFileName "${VTK_RESULTS_PATH}$afile.error.tif"
	rttiffw Write
	vtkTIFFWriter rttiffw2
	rttiffw2 SetInput [w2if GetOutput]
	rttiffw2 SetFileName "${VTK_RESULTS_PATH}$afile.test.tif"
	rttiffw2 Write
    }

    # Write the image error out to the log file
    puts -nonewline $logFile "$imageError imgdiff, "

    # Put the final passed or failed flag out there.
    # If it failed, say why (Image, Time)
    if { $imageStatus == "Passed" } {
	puts -nonewline $logFile "Passed    "
	rtLog DisplayTag "<Passed>true</Passed>"
    } else {
	puts $logFile "Failed (Image)"
	rtLog DisplayTag "<Passed>false</Passed>"
    }

    set retval [CheckTime $afile [string trimleft $CPUTime]]

    if  { $imageStatus == "Passed" } {
	puts $logFile $retval
    }

    if { $retval != "" && $retval != "Warning: New Test" && $retval != "Warning: Recently Added Test" } {
	GeneratePlotFiles $afile [string trimleft $CPUTime]
    }
    
    # reset global GL variables
    vtkPolyDataMapper rtMapper
      rtMapper SetResolveCoincidentTopologyToDefault
      rtMapper GlobalImmediateModeRenderingOff

    rtLog DisplayTag "<EndDateTime>[clock format [clock seconds]]</EndDateTime>"
    rtLog DisplayTag "</TestRun>"
    vtkCommand DeleteAllObjects
    catch {destroy .top}
    catch {destroy .geo}
}

exit $returnStatus
