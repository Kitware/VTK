
# setup some common things for testing
vtkObject rtTempObject

vtkMath rtExMath
rtExMath RandomSeed 6

# create the testing class to do the work
vtkTesting rtTester
for {set i 1} {$i < $argc} {incr i} {
   rtTester AddArgument "[lindex $argv $i]"
}
set VTK_DATA_ROOT [rtTester GetDataRoot]

for {set i  1} {$i < [expr $argc - 1]} {incr i} {
   if {[lindex $argv $i] == "-A"} {
      foreach dir [split [lindex $argv [expr $i +1]] ":"] {
         lappend auto_path $dir
      }
   }
}

# load in the script
set file [lindex $argv 0]

# set the default threshold, the Tcl script may change this
set threshold -1

if {[info commands wm] != ""} {
  wm withdraw .
} else {
  # There is no Tk.  Help the tests run without it.
  proc wm args {
    puts "wm not implemented"
    }
  # The vtkinteraction package requires Tk but since Tk is not
  # available it will never be used anyway.  Just pretend it is
  # already loaded so that tests that load it will not try to load Tk.
  package provide vtkinteraction 5.3
}

# Run the test.
source $file
if {[info commands iren] == "iren"} {renWin Render}
# run the event loop quickly to map any tkwidget windows
update

# current directory
set rtResult 0
if {[rtTester IsValidImageSpecified] != 0} {
   # look for a renderWindow ImageWindow or ImageViewer
   # first check for some common names
   if {[info commands renWin] == "renWin"} {
      rtTester SetRenderWindow renWin
      if {$threshold == -1} {
         set threshold 10
      }
   } else {
      if {$threshold == -1} {
         set threshold 5
      }
      if {[info commands viewer] == "viewer"} {
         rtTester SetRenderWindow [viewer GetRenderWindow]
         viewer Render
      } else {
         if {[info commands imgWin] == "imgWin"} {
            rtTester SetRenderWindow imgWin
            imgWin Render
         } else {
            if {[info exists viewer]} {
               rtTester SetRenderWindow [$viewer GetRenderWindow]
            }
         }
      }
   }
   set rtResult [rtTester RegressionTest $threshold]
}

if {[rtTester IsInteractiveModeSpecified] != 0} {
  if {[info commands iren] == "iren"} {
    iren Start
  }
}

vtkCommand DeleteAllObjects
catch {destroy .top}
catch {destroy .geo}

if {$rtResult == 0} {exit 1}
exit 0
