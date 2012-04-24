package require vtk
package require vtkinteraction

# Create two polygon sources, one a closed polyline, one a polygon
#
vtkRegularPolygonSource polyline
polyline SetCenter 1 1 1
polyline SetRadius 1
polyline SetNumberOfSides 12
polyline SetNormal 1 2 3
polyline GeneratePolylineOn
polyline GeneratePolygonOff

vtkPolyDataMapper polylineMapper
polylineMapper SetInputConnection [polyline GetOutputPort]

vtkActor polylineActor
polylineActor SetMapper polylineMapper
[polylineActor GetProperty] SetColor 0 1 0
[polylineActor GetProperty] SetAmbient 1


vtkRegularPolygonSource polygon
polygon SetCenter 3 1 1
polygon SetRadius 1
polygon SetNumberOfSides 12
polygon SetNormal 1 2 3
polygon GeneratePolylineOff
polygon GeneratePolygonOn

vtkPolyDataMapper polygonMapper
polygonMapper SetInputConnection [polygon GetOutputPort]

vtkActor polygonActor
polygonActor SetMapper polygonMapper
[polygonActor GetProperty] SetColor 1 0 0
[polygonActor GetProperty] SetAmbient 1

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create room profile# Add the actors to the renderer, set the background and size
#
ren1 AddActor polylineActor
ren1 AddActor polygonActor

ren1 SetBackground 0 0 0
renWin SetSize 200 200
renWin Render

iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .

