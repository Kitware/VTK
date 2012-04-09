package require vtk
package require vtkinteraction

vtkSimplePointsReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/points.txt"

vtkPolyDataMapper mapper
  mapper SetInputConnection [reader GetOutputPort]

vtkActor actor
  actor SetMapper mapper
  [actor GetProperty] SetPointSize 5

vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor actor

renWin SetSize 300 300
iren Initialize
renWin Render
iren AddObserver UserEvent {wm deiconify .vtkInteract}
wm withdraw .
