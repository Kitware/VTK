package require vtkgraphics
package require vtkimaging

catch {
    unset __tk_error
}

if {[catch {
    package require Tk
} errormsg]} {
    if {[::vtk::load_component tk] != ""} {
        puts $errormsg
        puts "Tk was not found: the VTK rendering package can not be used... Please check that your Tcl/Tk installation is correct. Windows users should also check that the program used to open/execute Tcl files is the Tk shell (wish), not the Tcl shell (tclsh)."
        set __tk_error 1
    }
}

if {![info exists __tk_error] && \
        ([info commands vtkRenderWindow] != "" || \
        [::vtk::load_component vtkRenderingTCL] == "")} {

    # Set the default exit method of vtkWin32RenderWindowInteractor to
    # call the Tcl 'exit' command

    if {[info commands vtkWin32RenderWindowInteractor] != ""} {
        if {[catch {
            vtkWin32RenderWindowInteractor __temp_vtkwin32iren__
            __temp_vtkwin32iren__ SetClassExitMethod exit
            __temp_vtkwin32iren__ Delete
        } errormsg]} {
            puts $errormsg
        }
    }

    package provide vtkrendering 4.0
}
