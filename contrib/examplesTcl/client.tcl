catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

vtkSocketController sock
sock Initialize

vtkInputPort iPort
iPort SetController sock
iPort SetTag 11
iPort SetRemoteProcessId 1

vtkPolyDataMapper mapper
mapper SetInput [iPort GetPolyDataOutput]

vtkActor actor
actor SetMapper mapper

vtkRenderer renderer
renderer AddActor actor

vtkRenderWindow renWin
renWin AddRenderer renderer

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

wm withdraw .
sock ConnectTo localhost 11000

renWin Render