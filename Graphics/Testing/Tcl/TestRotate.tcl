package require vtk
package require vtkinteraction

vtkConeSource cone
cone SetRadius 0.05
cone SetHeight 0.25
cone SetResolution 256
cone SetCenter 0.15 0.0 0.15

vtkRotationFilter rotate
rotate SetInputConnection [cone GetOutputPort]
rotate SetAxisToZ
rotate SetCenter 0.0 0.0 0.0
rotate SetAngle 45
rotate SetNumberOfCopies 7
rotate CopyInputOn

vtkDataSetMapper mapper
mapper SetInputConnection [rotate GetOutputPort]

vtkActor actor
actor SetMapper mapper

vtkRenderer ren1
ren1 AddActor actor

vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetSize 512 512

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

renWin Render

wm withdraw .
