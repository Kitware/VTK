package provide vtkbase 4.2

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

    proc load_component {name {optional_paths {}}} {
        
        global tcl_platform auto_path env
        
        # First dir is empty, to let Tcl try in the current dir
        
        set dirs $optional_paths
        set dirs [concat $dirs {""}]
        set ext [info sharedlibextension]
        if {$tcl_platform(platform) == "unix"} {
            set prefix "lib"
            # Help Unix a bit by browsing into $auto_path and /usr/lib...
            set dirs [concat $dirs /usr/local/lib /usr/local/lib/vtk $auto_path]
            if {[info exists env(LD_LIBRARY_PATH)]} {
                set dirs [concat $dirs [split $env(LD_LIBRARY_PATH) ":"]]
            }
            if {[info exists env(PATH)]} {
                set dirs [concat $dirs [split $env(PATH) ":"]]
            }
        } else {
            set prefix ""
            if {$tcl_platform(platform) == "windows"} {
                if {[info exists env(PATH)]} {
                    set dirs [concat $dirs [split $env(PATH) ";"]]
                }
            }
        }

        foreach dir $dirs {
            set libname [file join $dir ${prefix}${name}${ext}]
            if {[file exists $libname]} {
                if {![catch {load $libname} errormsg]} {
                    # WARNING: it HAS to be "" so that pkg_mkIndex work (since
                    # while evaluating a package ::vtk::load_component won't
                    # exist and will default to the unknown() proc that 
                    # returns ""
                    return ""
                } elseif {$::vtk::complain_on_loading} {
                    # If not loaded but file was found, oops
                    puts stderr $errormsg
                }
            }
        }

        if {$::vtk::complain_on_loading} {
            puts stderr "::vtk::load_component: $name could not be found."
        }
        
        return 1
    }

    # Function returning either a command line argument, an environment 
    # variable or a default value.

    proc get_arg_or_env_or_default {arg envvar def} {
        
        # Look at command line args
        
        global argc argv
        if {[info exists argc]} { 
            set argcm1 [expr $argc - 1]
            for {set i 0} {$i < $argcm1} {incr i} {
                if {[lindex $argv $i] == $arg && $i < $argcm1} {
                    return [lindex $argv [expr $i + 1]]
                }
            }
        }

        # Look at environment vars

        global env
        if {[info exists env($envvar)]} {
            return $env($envvar)
        }

        # Return default

        return $def
    }

    # Get VTK_DATA_ROOT if we can

    variable VTK_DATA_ROOT [::vtk::get_arg_or_env_or_default \
            "-D" \
            "VTK_DATA_ROOT" \
            [file nativename [file join [file dirname [info script]] "../../../../VTKData"]]]
}

set VTK_DATA_ROOT $::vtk::VTK_DATA_ROOT
