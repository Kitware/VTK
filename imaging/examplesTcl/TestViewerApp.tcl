catch {load vtktcl}

source ViewerApp.tcl

vtkImageWindow imgWin

gradient BypassOff
magnitude BypassOff

$viewer Render
ResetTkImageViewer .top.f1.v1 


