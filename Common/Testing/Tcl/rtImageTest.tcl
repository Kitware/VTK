

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


if {$tcl_platform(os) == "Windows NT"} {
    vtkObject rtTempObject;
    rtTempObject GlobalWarningDisplayOff;
}

vtkMath rtExMath
rtExMath RandomSeed 6

# load inthe script
set file [lindex $argv 0]
#catch {source $file; if {[info commands iren] == "iren"} {renWin Render}}
source $file; if {[info commands iren] == "iren"} {renWin Render}

# run the event loop quickly to map any tkwidget windows
wm withdraw .
update

# current directory
if {$argc >= 3 && [lindex $argv [expr $argc - 2]] == "-V"} {
   
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
   set validImage [lindex $argv [expr $argc -1]]
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
