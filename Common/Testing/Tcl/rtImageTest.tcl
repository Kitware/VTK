
puts "Loading rtImageTest.tcl"

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


vtkObject rtTempObject;
rtTempObject GlobalWarningDisplayOff;

vtkMath rtExMath
rtExMath RandomSeed 6

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

set validImageFound 0
for {set i  1} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      set auto_path "$auto_path [lindex $argv [expr $i +1]]"
   }
   if {[lindex $argv $i] == "-V"} {
      set validImageFound 1
      set validImage "$VTK_DATA_ROOT/[lindex $argv [expr $i + 1]]"
   }
}

#catch {source $file; if {[info commands iren] == "iren"} {renWin Render}}
source $file; if {[info commands iren] == "iren"} {renWin Render}

# run the event loop quickly to map any tkwidget windows
wm withdraw .
update

# current directory
if {$validImageFound != 0} {
   
   vtkWindowToImageFilter rt_w2if
   # look for a renderWindow ImageWindow or ImageViewer
   # first check for some common names
   if {[info commands renWin] == "renWin"} {
      rt_w2if SetInput renWin
      set threshold 10
   } else {
      set threshold 0
      if {[info commands viewer] == "viewer"} {
         rt_w2if SetInput [viewer GetImageWindow]
         viewer Render
      } else {
         if {[info commands imgWin] == "imgWin"} {
            rt_w2if SetInput imgWin
            imgWin Render
         } else {
            if {[info exists viewer]} {
               rt_w2if SetInput [$viewer GetImageWindow]
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
         puts "Unable to find valid image!!!"
         vtkCommand DeleteAllObjects
         catch {destroy .top}
         catch {destroy .geo}
         exit 1
      }
   }
   
   vtkPNGReader rt_png
   rt_png SetFileName $validImage
   vtkImageDifference rt_id
   
   if {$threshold == 0} {rt_id AllowShiftOff; rt_id SetThreshold 1}
   rt_id SetInput [rt_w2if GetOutput]
   rt_id SetImage [rt_png GetOutput]
   rt_id Update
   set imageError [decipadString [rt_id GetThresholdedError] 4 9]
   rt_w2if Delete 
   rt_png Delete 
   if {[rt_id GetThresholdedError] > $threshold} {
      puts "Failed Image Test with error: $imageError"
      vtkCommand DeleteAllObjects
      catch {destroy .top}
      catch {destroy .geo}
      exit 1; 
   } 
}


vtkCommand DeleteAllObjects
catch {destroy .top}
catch {destroy .geo}

exit 0
