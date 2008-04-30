package require -exact vtkgraphics 5.2
package require -exact vtkimaging 5.2

catch {
    unset __tk_error
}

namespace eval ::vtk::rendering {
  proc SetWin32ExitCallback {} {
    # Set the default exit method of vtkWin32RenderWindowInteractor to
    # call the Tcl 'exit' command
    if {[info commands vtkWin32RenderWindowInteractor] != ""} {
      if {[catch {
        # this exit method is called when the user exists interactively
        # (such as pressing 'e' or 'q'). An extra reference is added to 
        # the callback command used to process it by the
        # vtkSubjectHelper::InvokeEvent method. If the callback exits the
        # application immediately in its Execute() method, this reference
        # is never removed and the object leaks. Using 'after idle' allows
        # the stack to unwind far enough. This is similar to Win32
        # PostQuitMessage() logic.
        vtkWin32RenderWindowInteractor __temp_vtkwin32iren__
        __temp_vtkwin32iren__ SetClassExitMethod {after idle exit}
        __temp_vtkwin32iren__ Delete
      } errormsg]} {
        puts $errormsg
      }
    }
  }
}

if {[info commands ::vtk::init::require_package] != ""} {
  if {![info exists __tk_error] && \
       [::vtk::init::require_package vtkRenderingTCL 5.2]} {
    ::vtk::rendering::SetWin32ExitCallback
    package provide vtkrendering 5.2
  }
} else {
  if {![info exists __tk_error] && \
        ([info commands vtkAxisActor2D] != "" || \
        [::vtk::load_component vtkRenderingTCL] == "")} {
    ::vtk::rendering::SetWin32ExitCallback
    package provide vtkrendering 5.2
  }
}
