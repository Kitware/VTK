package provide vtkbase 4.0

namespace eval vtk {


  proc load_tk {} {
    if { [lsearch [package names] Tk] == -1 } {
      ::vtk::load_componont tk
    }
  }
  
# load_component: load a VTK component 
#        Example: ::vtk::load_component vtkFilteringTCL

# Windows: the 'load' command looks in the same dir as the Tcl/Tk app,
#          the current dir, c:\window\system[32], c:\windows and the dirs 
#          listed in the PATH env var.
#    Unix: the 'load' command looks in dirs listed in the 
#          LD_LIBRARY_PATH env var.

proc load_component {name} {
  global tcl_platform auto_path env
  # First dir is empty, to let Tcl try a relative name
  set dirs {""}
  set ext [info sharedlibextension]
  if {$tcl_platform(platform) == "unix"} {
    set prefix "lib"
    # Help Unix a bit by browsing into $auto_path and /usr/lib...
    set dirs [concat $dirs /usr/local/lib $auto_path [split $env(LD_LIBRARY_PATH) ":"]]
  } else {
    set prefix ""
  }
  foreach dir $dirs {
    set libname [file join $dir ${prefix}${name}${ext}]
    if {![catch {load $libname} errormsg]} {
      return
    }
    # If not loaded but file was found, return immediately
    if {[file exists $libname]} {
      puts stderr $errormsg
    }
  }
  puts stderr "::vtk::load_component: $name could not be found!"
}

# I won't 'rename ::vtk::load_component ""' because it will be 
# quite useful for user-defined packages depending on vtktcl.

# set VTK_DATA if we can, first look at environment vars
proc SetVTK_DATA {} {
  global argc argv
  if { ![info exists argc] } { set argc 0; set argv {} }
  if { [catch {set VTK_DATA_ROOT $env(VTK_DATA_ROOT)}] != 0} { 
    # then look at command line args
    set vtkDataFound 0
    for {set i 0} {$i < [expr $argc - 1]} {incr i} {
      if {[lindex $argv $i] == "-D"} {
	set vtkDataFound 1
	set VTK_DATA_ROOT [lindex $argv [expr $i + 1]]
      }
    }
    # make a final guess at a relativepath
    if {$vtkDataFound == 0} {
      set VTK_DATA_ROOT "../../../../VTKData" 
    }
  }
}

variable DATA_ROOT [SetVTK_DATA]
}

set VTK_DATA_ROOT $::vtk::DATA_ROOT