
proc IncrementFileName { validImage count } {
    set res ""
    regsub {\.png} $validImage _${count}.png res
    return $res
}

vtkObject rtTempObject;
rtTempObject GlobalWarningDisplayOff;

vtkMath rtExMath
rtExMath RandomSeed 6

vtkDebugLeaks rtDebugLeaks
rtDebugLeaks PromptUserOff

set myProcId 0
set numProcs 1
vtkCompositeManager compManager
if { [compManager GetController] != "" } {
    catch [ set myProcId [[compManager GetController] GetLocalProcessId] ]
    catch [ set numProcs [[compManager GetController] GetNumberOfProcesses] ]
}

proc ExitMaster { code } {
    global numProcs
    for { set i 1 } { $i < $numProcs } { incr i } {
        # Send break to all the nodes
        #puts "Send break to: $i"
        catch [ [ compManager GetController ] TriggerRMI $i 239954 ]
    }
    
    vtkCommand DeleteAllObjects
    catch {destroy .top}
    catch {destroy .geo}
    
    exit $code
}

# load in the script
set file [lindex $argv 0]

# parse command line args
if { [catch {set VTK_DATA_ROOT $env(VTK_DATA_ROOT)}] != 0} { 
   # then look at command line args
   set vtkDataFound 0
   for {set i 1} {$i < [expr $argc - 1]} {incr i} {
      if {[lindex $argv $i] == "-D"} {
         set vtkDataFound 1
         set VTK_DATA_ROOT [lindex $argv [expr $i + 1]]
      }
   }
   # make a final guess at a relativepath
   if {$vtkDataFound == 0} then {set VTK_DATA_ROOT "../../../../VTKData" }
}

if { [catch {set VTK_BASELINE_ROOT $env(VTK_BASELINE_ROOT)}] != 0} { 
   # then look at command line args
   set vtkBaselineFound 0
   for {set i 1} {$i < [expr $argc - 1]} {incr i} {
      if {[lindex $argv $i] == "-B"} {
         set vtkBaselineFound 1
         set VTK_BASELINE_ROOT [lindex $argv [expr $i + 1]]
      }
   }
   # make a final guess at a relativepath
   if {$vtkBaselineFound == 0} then {set VTK_BASELINE_ROOT $VTK_DATA_ROOT }
}

set validImageFound 0
for {set i  1} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      linsert auto_path 0 [lindex $argv [expr $i +1]]
   }
   if {[lindex $argv $i] == "-V"} {
      set validImageFound 1
      set validImage "$VTK_BASELINE_ROOT/[lindex $argv [expr $i + 1]]"
   }
}

set threshold -1

if { $myProcId != 0 } {
    #puts "Start reading script on satellite node"
    source $file
    
    compManager InitializeRMIs
    #puts "Process RMIs"
    [ compManager GetController ] ProcessRMIs

    #puts "**********************************"
    #puts "Done on the slave node"
    #puts "**********************************"


    vtkCommand DeleteAllObjects
    catch {destroy .top}
    catch {destroy .geo}
    exit 0
}

# Create a timer to get CPU time.  Use tcl time command to get wall time.
vtkTimerLog rtTimer
set rtStartCPU [rtTimer GetCPUTime]
#catch {source $file; if {[info commands iren] == "iren"} {renWin Render}}
set rtRawWallTimeResult [time {
    source $file; if {[info commands iren] == "iren"} {renWin Render}

    # run the event loop quickly to map any tkwidget windows
    wm withdraw .
    update
} 1]
set rtEndCPU [rtTimer GetCPUTime]
set rtCPUTime [expr $rtEndCPU - $rtStartCPU]
set rtWallTime [expr [lindex $rtRawWallTimeResult 0] / 1000000.0]

# output measurements for Dart
puts -nonewline "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">"
puts -nonewline "$rtWallTime"
puts "</DartMeasurement>"
puts -nonewline "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">"
puts -nonewline "$rtCPUTime"
puts "</DartMeasurement>"

# current directory
if {$validImageFound != 0} {

   vtkWindowToImageFilter rt_w2if
   rt_w2if ReadFrontBufferOff
   # look for a renderWindow ImageWindow or ImageViewer
   # first check for some common names
   if {[info commands renWin] == "renWin"} {
      rt_w2if SetInput renWin
       if {$threshold == -1} {
	   set threshold 10
       }
   } else {
       if {$threshold == -1} {
	   set threshold 5
       }
      if {[info commands viewer] == "viewer"} {
         rt_w2if SetInput [viewer GetRenderWindow]
         viewer Render
      } else {
         if {[info commands imgWin] == "imgWin"} {
            rt_w2if SetInput imgWin
            imgWin Render
         } else {
            if {[info exists viewer]} {
               rt_w2if SetInput [$viewer GetRenderWindow]
            }
         }
      }
   }
   
   # does the valid image exist ?
   if {[file exists ${validImage}] == 0 } {
      if {[catch {set channel [open ${validImage} w]}] == 0 } {
         close $channel
         vtkPNGWriter rt_pngw
         rt_pngw SetFileName $validImage
         rt_pngw SetInput [rt_w2if GetOutput]
         rt_pngw Write
      } else {
         puts "Unable to find valid image:${validImage}"
         ExitMaster 1
      }
   }
   
   vtkPNGReader rt_png
   rt_png SetFileName $validImage
   vtkImageDifference rt_id
   
   rt_id SetInput [rt_w2if GetOutput]
   rt_id SetImage [rt_png GetOutput]
   rt_id Update
   set imageError [rt_id GetThresholdedError]
   set minError [rt_id GetThresholdedError]
   set bestImage $validImage
   rt_w2if Delete 

   set count 0
   set errIndex 0
   if {$minError > $threshold} {
       set count 1
       set testFailed 1
       set errIndex -1
       while 1 {
	   set newFileName [IncrementFileName $validImage $count]
	   if {[catch {set channel [open $newFileName r]}]} {
	       break
	   }
	   close $channel
	   rt_png SetFileName $newFileName
	   rt_png Update
	   rt_id Update
	   set altError [rt_id GetThresholdedError]
	   if { $altError <= $threshold } { 
	       # Test passed with the alternate image
	       set errIndex $count
	       set testFailed 0
	       set minError $altError
	       set imageError $altError
	       set bestImage $newFileName
	       break
	   } else {
	       # Test failed but is it better than any image we saw so far?
	       if { $altError < $minError } {
		   set errIndex $count
		   set minError $altError
		   set imageError $altError
		   set bestImage $newFileName
	       }
	   }
	   incr count 1
       }

       if { $testFailed } {
	   rt_png SetFileName $bestImage

	   rt_png Update
	   rt_id Update
	   
	   if {[catch {set channel [open $validImage.diff.png w]}] == 0 } {
	       close $channel
	       
               # write out the difference image in full resolution
	       vtkPNGWriter rt_pngw2
	       rt_pngw2 SetFileName $validImage.diff.png
	       rt_pngw2 SetInput [rt_id GetOutput]
	       rt_pngw2 Write 
               
               # write out the difference image scaled and gamma adjusted
               # for the dashboard
               set rt_size [[rt_png GetOutput] GetDimensions]
               if { [lindex $rt_size 1] <= 250.0} {
                   set rt_magfactor 1.0
               } else {
                   set rt_magfactor [expr 250.0 / [lindex $rt_size 1]]
               }

	       vtkImageResample rt_shrink
               rt_shrink SetInput [rt_id GetOutput]
               rt_shrink InterpolateOn
               rt_shrink SetAxisMagnificationFactor 0 $rt_magfactor 
               rt_shrink SetAxisMagnificationFactor 1 $rt_magfactor 

               vtkImageShiftScale rt_gamma
               rt_gamma SetInput [rt_shrink GetOutput]
               rt_gamma SetShift 0
               rt_gamma SetScale 10

               vtkJPEGWriter rt_jpegw_dashboard
               rt_jpegw_dashboard SetFileName $validImage.diff.small.jpg
               rt_jpegw_dashboard SetInput [rt_gamma GetOutput]
               rt_jpegw_dashboard SetQuality 85
               rt_jpegw_dashboard Write

	       # write out the image that was generated
	       rt_shrink SetInput [rt_id GetInput]
	       rt_jpegw_dashboard SetInput [rt_shrink GetOutput]
	       rt_jpegw_dashboard SetFileName $validImage.test.small.jpg
	       rt_jpegw_dashboard Write

	       # write out the valid image that matched
	       rt_shrink SetInput [rt_id GetImage]
	       rt_jpegw_dashboard SetInput [rt_shrink GetOutput]
	       rt_jpegw_dashboard SetFileName $validImage.small.jpg
	       rt_jpegw_dashboard Write

	   }
	   puts "Failed Image Test with error: $imageError"

	   puts -nonewline "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">"
	   puts -nonewline "$imageError"
	   puts "</DartMeasurement>"

	   if { $errIndex <= 0} {
	       puts -nonewline "<DartMeasurement name=\"BaselineImage\" type=\"text/string\">Standard</DartMeasurement>"
	   } else {
	       puts -nonewline "<DartMeasurement name=\"BaselineImage\" type=\"numeric/integer\">"
	       puts -nonewline "$errIndex"
	       puts "</DartMeasurement>"
	   }
	   
	   puts -nonewline "<DartMeasurementFile name=\"TestImage\" type=\"image/jpeg\">"
	   puts -nonewline "$validImage.test.small.jpg"
	   puts "</DartMeasurementFile>"

	   puts -nonewline "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/jpeg\">"
	   puts -nonewline "$validImage.diff.small.jpg"
	   puts "</DartMeasurementFile>"

	   puts -nonewline "<DartMeasurementFile name=\"ValidImage\" type=\"image/jpeg\">"
	   puts -nonewline "$validImage.small.jpg"
	   puts "</DartMeasurementFile>"
	   
	   ExitMaster 1; 
       }
   }
   
   # output the image error even if a test passed
   puts -nonewline "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">"
   puts -nonewline "$imageError"
   puts "</DartMeasurement>"

   if { $errIndex <= 0} {
       puts -nonewline "<DartMeasurement name=\"BaselineImage\" type=\"text/string\">Standard</DartMeasurement>"
   } else {
       puts -nonewline "<DartMeasurement name=\"BaselineImage\" type=\"numeric/integer\">"
       puts -nonewline "$errIndex"
       puts "</DartMeasurement>"
   }
}


ExitMaster 0
