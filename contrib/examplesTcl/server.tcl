catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

vtkSocketController sock
sock Initialize

vtkConeSource cone

vtkOutputPort oPort
oPort SetInput [cone GetOutput]
oPort SetController sock
oPort SetTag 11

sock WaitForConnection 11000 0

oPort WaitForUpdate
