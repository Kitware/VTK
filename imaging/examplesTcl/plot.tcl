# This example demonstrates how to use some plotting objects.
catch {load vtktcl}

# get the interactor ui
source ../../examplesTcl/vtkInt.tcl

# Create a 1D axis
vtkAxisActor2D axis
    axis SetNumberOfLabels 5
    axis SetTitle "X-Axis"
    [axis GetPoint1Coordinate] SetCoordinateSystemToNormalizedViewport
    [axis GetPoint1Coordinate] SetValue 0.25 0.25
    [axis GetPoint2Coordinate] SetCoordinateSystemToNormalizedViewport
    [axis GetPoint2Coordinate] SetValue 0.75 0.25

vtkAxisActor2D axis2
    axis2 SetNumberOfLabels 5
    axis2 SetTitle "Y-Axis"
    [axis2 GetPoint1Coordinate] SetCoordinateSystemToNormalizedViewport
    [axis2 GetPoint1Coordinate] SetValue 0.25 0.75
    [axis2 GetPoint2Coordinate] SetCoordinateSystemToNormalizedViewport
    [axis2 GetPoint2Coordinate] SetValue 0.25 0.25
    axis2 SetRange 1 0
    axis2 SetFontFactor 0.8

vtkAxisActor2D axis3
    axis3 SetNumberOfLabels 5
    axis3 SetTitle "Z-Axis"
    [axis3 GetPoint1Coordinate] SetCoordinateSystemToNormalizedViewport
    [axis3 GetPoint1Coordinate] SetValue 0.3 0.3
    [axis3 GetPoint2Coordinate] SetCoordinateSystemToNormalizedViewport
    [axis3 GetPoint2Coordinate] SetValue 0.8 0.8
    axis3 SetRange -2.4 6.7
    axis3 ShadowOff
    [axis3 GetProperty] SetColor 0 1 0

# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

ren1 AddActor2D axis
ren1 AddActor2D axis2
ren1 AddActor2D axis3
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render
iren Initialize

#renWin SetFileName "plot.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .

