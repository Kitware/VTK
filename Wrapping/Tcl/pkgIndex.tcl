# Tcl package index file, version 1.0

namespace eval ::vtktcl {
    variable package_index_dir [file dirname [info script]] 
}

package ifneeded vtktcl 4.0 {

    # load_component: load a VTK component 
    #        Example: ::vtktcl::load_component vtkFilteringTCL
    
    # Windows: the 'load' command looks in the same dir as the Tcl/Tk app,
    #          the current dir, c:\window\system[32], c:\windows and the dirs 
    #          listed in the PATH env var.
    #    Unix: the 'load' command looks in dirs listed in the 
    #          LD_LIBRARY_PATH env var.

    proc ::vtktcl::load_component {name} {
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
        return "::vtktcl::load_component: $name could not be found!"
    }

    # I won't 'rename ::vtktcl::load_component ""' because it will be 
    # quite useful for user-defined packages depending on vtktcl.

    # load_script: load an additional VTK-related script (located in the 
    #              same place as this package index file is).
    #     Example: ::vtktcl::load_script mccases

    proc ::vtktcl::load_script {name} {
        set filenames [list \
                [file join $::vtktcl::package_index_dir $name] \
                [file join $::vtktcl::package_index_dir $name.tcl] \
                ]
        foreach filename $filenames {
            if {[file exists $filename]} {
                uplevel source $filename
                return 1
            }
        }
        puts "::vtktcl::load_script: $name could not be found!"
        return 0
    }

    set ok 1

    # Check if vtk commands are already there and try to
    # load the libraries only if they are not

    if {[catch {vtkCommand ListAllInstances}]} {
        # Try to load at least the Common part
        set errormsg [::vtktcl::load_component vtkCommonTCL]
        if {$errormsg != ""} {
            puts $errormsg
            set ok 0
        } else {
            # Try to load the other components
            ::vtktcl::load_component vtkFilteringTCL
            ::vtktcl::load_component vtkGraphicsTCL
            ::vtktcl::load_component vtkImagingTCL
            ::vtktcl::load_component vtkHybridTCL
            ::vtktcl::load_component vtkIOTCL
            ::vtktcl::load_component vtkParallelTCL
            ::vtktcl::load_component vtkPatentedTCL
            ::vtktcl::load_component vtkRenderingTCL
        }
    }
    
    # Try to set the exit method of the interactor (needs Rendering)

    if {[info commands vtkWin32RenderWindowInteractor] != ""} {
        if {[catch {
            vtkWin32RenderWindowInteractor __temp_vtkwin32iren__
            __temp_vtkwin32iren__ SetClassExitMethod exit
            __temp_vtkwin32iren__ Delete
        } errormsg ]} {
            # warn the user
            puts $errormsg
        }
    }
    
    # set VTK_DATA if we can, first look at environment vars

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

    if {$ok} {
        package provide vtktcl 4.0
    }
}

package ifneeded vtktcl_interactor 1.0 {

    package require vtktcl

    source [file join $::vtktcl::package_index_dir Interactor.tcl]

    package provide vtktcl_interactor 1.0
}


package ifneeded vtktcl_widgets 1.0 {
    package require vtktcl_interactor

    source [file join $::vtktcl::package_index_dir WidgetObject.tcl]
    source [file join $::vtktcl::package_index_dir TkInteractor.tcl]

    package provide vtktcl_widgets 1.0
}


# Please use:
#   ::vtktcl::load_script colors
#   ::vtktcl::load_script mccases
#   ::vtktcl::load_script backdrop
#   etc.
