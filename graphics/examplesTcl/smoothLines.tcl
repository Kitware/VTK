catch {load vtktcl}
# this is a tcl version to demonstrate line smoothing
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a semi-cylinder
#
vtkLineSource line
  line SetPoint1 0 0 1
  line SetPoint2 50 0 1
  line SetResolution 49

vtkBrownianPoints bump
  bump SetInput [line GetOutput]

vtkWarpVector warp
  warp SetInput [bump GetPolyDataOutput]
  warp SetScaleFactor .1

set iterations "0 10 20 30 40 50"
foreach iteration $iterations {
    
vtkSmoothPolyDataFilter smooth$iteration
    smooth$iteration SetInput [warp GetOutput]
    smooth$iteration SetNumberOfIterations $iteration
    smooth$iteration BoundarySmoothingOn
    smooth$iteration SetFeatureAngle 120
    smooth$iteration SetEdgeAngle 90
    smooth$iteration SetRelaxationFactor .025
    smooth$iteration GenerateErrorScalarsOn

vtkPolyDataMapper lineMapper$iteration
    lineMapper$iteration SetInput [smooth$iteration GetOutput]
    lineMapper$iteration SetScalarRange 0 .04

vtkActor lineActor$iteration
    lineActor$iteration SetMapper lineMapper$iteration
    eval [lineActor$iteration GetProperty] SetColor $beige

ren1 AddActor lineActor$iteration
lineActor$iteration AddPosition 0 $iteration 0
}

# Add the actors to the renderer, set the background and size
#


ren1 SetBackground 1 1 1
renWin SetSize 350 350

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

iren Initialize
renWin SetFileName "smoothLines.tcl.ppm"
renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
