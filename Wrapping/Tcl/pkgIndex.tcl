# Tcl package index file, version 1.0

package ifneeded vtktcl 3.3 {

    # Windows: 'load' looks in the same dir as the Tcl/Tk app, the current dir,
    # c:\window\system[32], c:\windows and the dirs listed in the PATH env var.
    # Unix: 'load' looks in dirs listed in the LD_LIBRARY_PATH env var.

    proc __temp_try_to_load_vtk_lib {name} {
        global tcl_platform
        # First dir is empty, to let Tcl try a relative name
        set dirs {""}
        set ext [info sharedlibextension]
        if {$tcl_platform(platform) == "unix"} {
            set prefix "lib"
            global auto_path
            # Help Unix a bit by browsing into $auto_path and /usr/lib...
            set dirs [concat $dirs /usr/local/lib $auto_path]
        } else {
            set prefix ""
        }
        foreach dir $dirs {
            set libname [file join $dir ${prefix}${name}${ext}]
            if {![catch {load $libname} errormsg]} {
                return ""
            }
            # If not loaded but file was found, return immediately
            if {[file exists $libname]} {
                return $errormsg
            }
        }
        return "$name could not be found!"
    }

    set ok 1

    # Check if vtk commands are already there and try to
    # load the libraries only if they are not
    if {[catch {vtkCommand ListAllInstances}]} {
        # Try to load at least the Common part
        set errormsg [__temp_try_to_load_vtk_lib vtkCommonTCL]
        if {$errormsg != ""} {
            puts $errormsg
            set ok 0
        } else {
            # Try to load the other components
            __temp_try_to_load_vtk_lib vtkFilteringTCL
            __temp_try_to_load_vtk_lib vtkGraphicsTCL
            __temp_try_to_load_vtk_lib vtkImagingTCL
            __temp_try_to_load_vtk_lib vtkHybridTCL
            __temp_try_to_load_vtk_lib vtkIOTCL
            __temp_try_to_load_vtk_lib vtkParallelTCL
            __temp_try_to_load_vtk_lib vtkPatentedTCL
            # Try to set the exit method of the interactor (needs Rendering)
            if {[__temp_try_to_load_vtk_lib vtkRenderingTCL] == ""} {
                if {$tcl_platform(platform) == "windows" && 
                [catch {
                    vtkWin32RenderWindowInteractor __temp_vtkwin32iren__
                    __temp_vtkwin32iren__ SetClassExitMethod exit
                    __temp_vtkwin32iren__ Delete
                } errormsg ]} {
                    # warn the user, but do not prevent to provide the package
                    puts $errormsg
                }
            }
        }
    }
        
    # set VTK_DATA if we can, first look at environment vars
    if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { 
       # then look at command line args
       set vtkDataFound 0
       for {set i 0} {$i < [expr $argc - 1]} {incr i} {
          if {[lindex $argv $i] == "-D"} {
             set vtkDataFound 1
             set VTK_DATA [lindex $argv [expr $i + 1]]
          }
       }
       # make a final guess at a relativepath
       if {$vtkDataFound == 0} {set VTK_DATA "../../../../VTKData" }
    }

    if {$ok} {
        package provide vtktcl 3.3
    }
}
