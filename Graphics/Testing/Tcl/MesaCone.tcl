package require vtk
package require vtkinteraction

vtkXMesaRenderWindow renWin

vtkMesaRenderer ren
renWin AddRenderer ren

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

vtkConeSource cone

vtkMesaPolyDataMapper mp
mp SetInput [cone GetOutput]

vtkMesaActor actor
actor SetMapper mp

ren AddActor actor

renWin Render
iren AddObserver UserEvent {wm deiconify .vtkInteract}

iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
