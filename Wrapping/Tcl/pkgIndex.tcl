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
       if {$vtkDataFound == 0} {set VTK_DATA_ROOT "../../../../VTKData" }
    }


    # a generic interactor for tcl and vtk
    #
    catch {unset vtkInteract.bold}
    catch {unset vtkInteract.normal}
    catch {unset vtkInteract.tagcount}
    set vtkInteractBold "-background #43ce80 -foreground #221133 -relief raised -borderwidth 1"
    set vtkInteractNormal "-background #dddddd -foreground #221133 -relief flat"
    set vtkInteractTagcount 1
    set vtkInteractCommandList ""
    set vtkInteractCommandIndex 0
    
    proc vtkInteract {} {
       global vtkInteractCommandList vtkInteractCommandIndex
       global vtkInteractTagcount
       
       proc dovtk {s w} {
          global vtkInteractBold vtkInteractNormal vtkInteractTagcount 
          global vtkInteractCommandList vtkInteractCommandIndex
          
          set tag [append tagnum $vtkInteractTagcount]
          set vtkInteractCommandIndex $vtkInteractTagcount
          incr vtkInteractTagcount 1
          .vtkInteract.display.text configure -state normal
          .vtkInteract.display.text insert end $s $tag
          set vtkInteractCommandList [linsert $vtkInteractCommandList end $s]
          eval .vtkInteract.display.text tag configure $tag $vtkInteractNormal
          .vtkInteract.display.text tag bind $tag <Any-Enter> \
                ".vtkInteract.display.text tag configure $tag $vtkInteractBold"
          .vtkInteract.display.text tag bind $tag <Any-Leave> \
                ".vtkInteract.display.text tag configure $tag $vtkInteractNormal"
          .vtkInteract.display.text tag bind $tag <1> "dovtk [list $s] .vtkInteract"
          .vtkInteract.display.text insert end \n;
          .vtkInteract.display.text insert end [uplevel 1 $s]
          .vtkInteract.display.text insert end \n\n
          .vtkInteract.display.text configure -state disabled
          .vtkInteract.display.text yview end
       }
       
       catch {destroy .vtkInteract}
       toplevel .vtkInteract -bg #bbbbbb
       wm title .vtkInteract "vtk Interactor"
       wm iconname .vtkInteract "vtk"
       
       frame .vtkInteract.buttons -bg #bbbbbb
       pack  .vtkInteract.buttons -side bottom -fill both -expand 0 -pady 2m
       button .vtkInteract.buttons.dismiss -text Dismiss \
             -command "wm withdraw .vtkInteract" \
             -bg #bbbbbb -fg #221133 -activebackground #cccccc -activeforeground #221133
       pack .vtkInteract.buttons.dismiss -side left -expand 1 -fill x
       
       frame .vtkInteract.file -bg #bbbbbb
       label .vtkInteract.file.label -text "Command:" -width 10 -anchor w \
             -bg #bbbbbb -fg #221133
       entry .vtkInteract.file.entry -width 40 \
             -bg #dddddd -fg #221133 -highlightthickness 1 -highlightcolor #221133
       bind .vtkInteract.file.entry <Return> {
          dovtk [%W get] .vtkInteract; %W delete 0 end}
          pack .vtkInteract.file.label -side left
          pack .vtkInteract.file.entry -side left -expand 1 -fill x
          
          frame .vtkInteract.display -bg #bbbbbb
          text .vtkInteract.display.text -yscrollcommand ".vtkInteract.display.scroll set" \
                -setgrid true -width 60 -height 8 -wrap word -bg #dddddd -fg #331144 \
                -state disabled
          scrollbar .vtkInteract.display.scroll \
                -command ".vtkInteract.display.text yview" -bg #bbbbbb \
                -troughcolor #bbbbbb -activebackground #cccccc -highlightthickness 0 
          pack .vtkInteract.display.text -side left -expand 1 -fill both
          pack .vtkInteract.display.scroll -side left -expand 0 -fill y
          
          pack .vtkInteract.display -side bottom -expand 1 -fill both
          pack .vtkInteract.file -pady 3m -padx 2m -side bottom -fill x 
          
          set vtkInteractCommandIndex 0
          
          bind .vtkInteract <Down> {
             if { $vtkInteractCommandIndex < [expr $vtkInteractTagcount - 1] } {
                incr vtkInteractCommandIndex
                set command_string [lindex $vtkInteractCommandList $vtkInteractCommandIndex]
                .vtkInteract.file.entry delete 0 end
                .vtkInteract.file.entry insert end $command_string
             } elseif { $vtkInteractCommandIndex == [expr $vtkInteractTagcount - 1] } {
                .vtkInteract.file.entry delete 0 end
             }
          }
          
          bind .vtkInteract <Up> {
             if { $vtkInteractCommandIndex > 0 } { 
                set vtkInteractCommandIndex [expr $vtkInteractCommandIndex - 1]
                set command_string [lindex $vtkInteractCommandList $vtkInteractCommandIndex]
                .vtkInteract.file.entry delete 0 end
                .vtkInteract.file.entry insert end $command_string
             }
          }
          
          wm withdraw .vtkInteract
       }
       vtkInteract
       
    if {$ok} {
        package provide vtktcl 3.3
    }
}
