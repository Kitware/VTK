
# setup some common things for testing
vtkObject rtTempObject;
rtTempObject GlobalWarningDisplayOff;
vtkMath rtExMath
rtExMath RandomSeed 6

# create the testing class to do the work
vtkTesting rtTester
for {set i 1} {$i < [expr $argc - 1]} {incr i} {
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

vtkMPIController mpc
set gc [mpc GetGlobalController]
mpc Delete

vtkCompositeRenderManager compManager

if { $gc != "" } {
    set myProcId [$gc GetLocalProcessId]
    set numProcs [$gc GetNumberOfProcesses]

    compManager SetController $gc
} else {
    set myProcId 0
    set numProcs 1
}


proc ExitMaster { code } {
    global numProcs
    for { set i 1 } { $i < $numProcs } { incr i } {
        # Send break to all the nodes
        #puts "Send break to: $i"
        set contr [ compManager GetController ]
        catch [ $contr TriggerRMI $i [$contr GetBreakRMITag] ]
    }
    
    vtkCommand DeleteAllObjects
    catch {destroy .top}
    catch {destroy .geo}
    
    exit $code
}

# load in the script
set file [lindex $argv 0]

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

if {$rtResult == 0} {ExitMaster 1}
ExitMaster 0
