package provide vtkbase 4.0

namespace eval ::vtk {

    namespace export *

    # load_component: load a VTK component 
    #        Example: ::vtk::load_component vtkFilteringTCL

    # Windows: the 'load' command looks for DLL in the Tcl/Tk dir,
    #          the current dir, c:\window\system[32], c:\windows and the dirs 
    #          listed in the PATH environment var.
    #    Unix: the 'load' command looks for shared libs in dirs listed in the 
    #          LD_LIBRARY_PATH environment var.

    variable complain_on_loading 1

    proc load_component {name} {

        global tcl_platform auto_path env

        # First dir is empty, to let Tcl try in the current dir

        set dirs {""}
        set ext [info sharedlibextension]
        if {$tcl_platform(platform) == "unix"} {
            set prefix "lib"
            # Help Unix a bit by browsing into $auto_path and /usr/lib...
            set dirs [concat $dirs /usr/local/lib /usr/local/lib/vtk $auto_path]
            if {[info exists env(LD_LIBRARY_PATH)]} {
                set dirs [concat $dirs [split $env(LD_LIBRARY_PATH) ":"]]
            }
        } else {
            set prefix ""
        }

        foreach dir $dirs {
            set libname [file join $dir ${prefix}${name}${ext}]
            if {![catch {load $libname} errormsg]} {
                # WARNING: it HAS to be "" so that pkg_mkIndex work (since
                # while evaluating a package ::vtk::load_component won't
                # exist and will default to the unknown() proc that returns ""
                return ""
            }
            # If not loaded but file was found, oops
            if {[file exists $libname] && $::vtk::complain_on_loading} {
                puts stderr $errormsg
            }
        }

        if {$::vtk::complain_on_loading} {
            puts stderr "::vtk::load_component: $name could not be found."
        }

        return 1
    }

    # Get VTK_DATA_ROOT if we can

    proc get_VTK_DATA_ROOT {} {

        # First look at environment vars

        global env
        if {[info exists env(VTK_DATA_ROOT)]} {
            return $env(VTK_DATA_ROOT)
        }

        # Then look at command line args

        global argc argv
        if {[info exists argc]} { 
            set argcm1 [expr $argc - 1]
            for {set i 0} {$i < $argcm1} {incr i} {
                if {[lindex $argv $i] == "-D" && $i < $argcm1} {
                    return [lindex $argv [expr $i + 1]]
                }
            }
        }

        # Make a final guess at a relativepath

        return [file nativename [file join [file dirname [info script]] "../../../../VTKData"]]
    }

    variable VTK_DATA_ROOT [get_VTK_DATA_ROOT]
}

set VTK_DATA_ROOT $::vtk::VTK_DATA_ROOT
