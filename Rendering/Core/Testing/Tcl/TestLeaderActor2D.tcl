package require vtk
package require vtkinteraction

# create pipeline
#
vtkLeaderActor2D leader
    [leader GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport
    [leader GetPositionCoordinate] SetValue 0.1 0.1
    [leader GetPosition2Coordinate] SetCoordinateSystemToNormalizedViewport
    [leader GetPosition2Coordinate] SetValue 0.75 0.23
    leader SetArrowStyleToFilled
    leader SetLabel ""
vtkLeaderActor2D leader2
    [leader2 GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport
    [leader2 GetPositionCoordinate] SetValue 0.9 0.1
    [leader2 GetPosition2Coordinate] SetCoordinateSystemToNormalizedViewport
    [leader2 GetPosition2Coordinate] SetValue 0.75 0.83
    leader2 SetArrowStyleToOpen
    leader2 SetArrowPlacementToPoint1
    leader2 SetLabel "Leader2"
vtkLeaderActor2D leader3
    [leader3 GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport
    [leader3 GetPositionCoordinate] SetValue 0.1 0.9
    [leader3 GetPosition2Coordinate] SetCoordinateSystemToNormalizedViewport
    [leader3 GetPosition2Coordinate] SetValue 0.6 0.3
    leader3 SetArrowStyleToHollow
    leader3 SetLabel "Leader3"
vtkLeaderActor2D leader4
    [leader4 GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport
    [leader4 GetPositionCoordinate] SetValue 0.1 0.75
    [leader4 GetPosition2Coordinate] SetCoordinateSystemToNormalizedViewport
    [leader4 GetPosition2Coordinate] SetValue 0.1 0.25
    leader4 SetArrowPlacementToNone
    leader4 SetRadius 1.0
    leader4 SetLabel "Leader4"
    leader4 AutoLabelOn
    
# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor leader
ren1 AddActor leader2
ren1 AddActor leader3
ren1 AddActor leader4
renWin SetSize 250 250

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

