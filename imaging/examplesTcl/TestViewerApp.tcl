catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source ViewerApp.tcl

vtkImageWindow imgWin

gradient BypassOff
magnitude BypassOff

$viewer Render
ResetTkImageViewer .top.f1.v1 


