package require vtkgraphics
package require vtkimaging

if {[catch {
    package require Tk
}]} {
    ::vtk::load_component tk
}

if {[::vtk::load_component vtkRenderingTCL] == ""} {

    # Set the default exit method of vtkWin32RenderWindowInteractor to
    # call the Tcl 'exit' command

    if {[info commands vtkWin32RenderWindowInteractor] != ""} {
        if {[catch {
            vtkWin32RenderWindowInteractor __temp_vtkwin32iren__
            __temp_vtkwin32iren__ SetClassExitMethod exit
            __temp_vtkwin32iren__ Delete
        } errormsg ]} {
            puts $errormsg
        }
    }
    
    package provide vtkrendering 4.0
}
