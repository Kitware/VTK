package require vtk
package require vtkinteraction

vtkRenderWindow renWin
renWin OffScreenRenderingOn

vtkRenderer ren
renWin AddRenderer ren

vtkConeSource cone

vtkPolyDataMapper mp
mp SetInputConnection [cone GetOutputPort]

vtkActor actor
actor SetMapper mp

ren AddActor actor

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
