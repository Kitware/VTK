package provide vtk 4.0

package require vtkbase
package require vtkcommon
package require vtkfiltering
package require vtkgraphics
package require vtkio
package require vtkimaging
package require vtkrendering
package require vtkhybrid
if { [catch { package require vtkpatented } Result ] } { puts stderr $Result }

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

