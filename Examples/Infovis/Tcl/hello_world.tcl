
package require vtk

vtkRandomGraphSource src

vtkGraphLayoutView view
view AddRepresentationFromInputConnection [src GetOutputPort]
view ResetCamera
set window [view GetRenderWindow]
[$window GetInteractor] Start
wm withdraw .

