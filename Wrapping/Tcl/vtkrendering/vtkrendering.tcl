package require -exact vtkgraphics 4.2
package require -exact vtkimaging 4.2

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

namespace eval ::vtk::rendering {
  proc SetWin32ExitCallback {} {
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
  }
}

if {[info commands ::vtk::init::require_package] != ""} {
  if {![info exists __tk_error] && \
       [::vtk::init::require_package vtkRenderingTCL 4.2]} {
    ::vtk::rendering::SetWin32ExitCallback
    package provide vtkrendering 4.2
  }
} else {
  if {![info exists __tk_error] && \
        ([info commands vtkAxisActor2D] != "" || \
        [::vtk::load_component vtkRenderingTCL] == "")} {
    ::vtk::rendering::SetWin32ExitCallback
    package provide vtkrendering 4.2
  }
}
