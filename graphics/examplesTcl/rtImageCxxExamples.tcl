catch {load vtktcl}
#
# This is a regression test script for VTK.
#

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

# first find all the examples. they can be defined on command line or in
# current directory
if { $argv != ""} {
    set files $argv
} else {
    set files [lsort [glob {[A-z]*}]]

}

# Determine which files are executable
set allFiles $files
foreach afile $allFiles {
  if { [file executable $afile] != 1 } {
    set pos [lsearch $files $afile]
    set files [lreplace $files $pos $pos ]
  }
}

# remove files that are not suitable for regression tests or simply don't 
# work right now
set noTest {
    VRMLServer XMace2 config.status configure configure.in CVS Makefile.in}

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


# now do the tests
foreach afile $files {
    #
    # only tcl scripts with valid/ images are tested
    set validImage $validPath/${afile}.cxx${VTK_PLATFORM}.tif
    
    #
    # first see if vtk has been built --with-patented
    if { [info command vtkMarchingCubes] == "" } {
	set validImage $validPath/$afile.cxx${VTK_PLATFORM}.withoutpatented.tif
	if {[catch {set channel [open ${validImage}]}] != 0 } {
            set validImage $validPath/$afile.cxx.withoutpatented.tif
            if {[catch {set channel [open ${validImage}]}] != 0 } {
                set validImage $validPath/$afile.cxx${VTK_PLATFORM}.tif
            } else {
                close $channel
            }
        } else {
            close $channel
        }
    }

    #
    # now see if there is an alternate image for this architecture
    if {[catch {set channel [open ${validImage}]}] != 0 } {
	set validImage $validPath/$afile.cxx.tif
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
    
    # Start by putting the name out - right justify it and pad to 30 characters.
    # This line MUST start with a space so that name conflicts won't occur in
    # grep statements in other files
    set Name [padString $afile 29]
    puts -nonewline $logFile " $Name - "
    flush stdout
    
    # Create a timer so that we can get CPU time.
    # Use the tcl time command to get wall time
    vtkTimerLog timer
    set startCPU [timer GetCPUTime]
    set wallTime [decipadString [expr [lindex [time {catch {puts "[exec $afile -S]"}} 1] 0] / 1000000.0] 4 9]
    set endCPU [timer GetCPUTime]
    set CPUTime [decipadString [expr $endCPU - $startCPU] 3 8]
    puts -nonewline $logFile "$wallTime wall, $CPUTime cpu, "
    
    set threshold 10

    # run the event loop quickly to map any tkwidget windows
    wm withdraw .
    
    vtkTIFFReader rttiff
    rttiff SetFileName ${validImage}
   
    vtkTIFFReader testimagetiff
    testimagetiff SetFileName ${afile}.cxx.tif
   
    vtkImageDifference imgDiff
    
    if {$threshold == 0} {imgDiff AllowShiftOff; imgDiff SetThreshold 1}
    imgDiff SetInput [testimagetiff GetOutput]
    imgDiff SetImage [rttiff GetOutput]
    imgDiff Update

    set imageError [decipadString [imgDiff GetThresholdedError] 4 9]

    # a test has to be off by at least threshold pixels for us to care   
    if {[imgDiff GetThresholdedError] <= $threshold} {
	set imageStatus "Passed"
    } else {
	set imageStatus "Failed"
	vtkTIFFWriter rttiffw
	rttiffw SetInput [imgDiff GetOutput]
	rttiffw SetFileName "${VTK_RESULTS_PATH}$afile.cxx.error.tif"
	rttiffw Write
	vtkTIFFWriter rttiffw2
	rttiffw2 SetInput [testimagetiff GetOutput]
	rttiffw2 SetFileName "${VTK_RESULTS_PATH}$afile.cxx.test.tif"
	rttiffw2 Write
    }

    # Write the image error out to the log file
    puts -nonewline $logFile "$imageError imgdiff, "

    # Put the final passed or failed flag out there.
    # If it failed, say why (Image, Time)
    if { $imageStatus == "Passed" } {
	puts $logFile "Passed"
    } else {
	puts $logFile "Failed (Image)"
    }

  

    vtkCommand DeleteAllObjects
    catch {destroy .top}
    catch {destroy .geo}
}

exit
