catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl

# First create the render master
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren   [$renWin MakeRenderWindowInteractor]

# cut data
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "../../../data/combxyz.bin"
    pl3d SetQFileName "../../../data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
vtkPlane plane
    eval plane SetOrigin [[pl3d GetOutput] GetCenter]
    plane SetNormal -0.287 0 0.9579
vtkCutter planeCut
    planeCut SetInput [pl3d GetOutput]
    planeCut SetCutFunction plane
vtkDataSetMapper cutMapper
    cutMapper SetInput [planeCut GetOutput]
    eval cutMapper SetScalarRange \
      [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange]
vtkActor cutActor
    cutActor SetMapper cutMapper

#extract plane
vtkStructuredGridGeometryFilter compPlane
    compPlane SetInput [pl3d GetOutput]
    compPlane SetExtent 0 100 0 100 9 9
vtkPolyMapper planeMapper
    planeMapper SetInput [compPlane GetOutput]
    planeMapper ScalarVisibilityOff
vtkActor planeActor
    planeActor SetMapper planeMapper
    [planeActor GetProperty] SetRepresentationToWireframe
    [planeActor GetProperty] SetColor 0 0 0

#outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
$ren1 AddActor outlineActor
$ren1 AddActor planeActor
$ren1 AddActor cutActor
$ren1 SetBackground 1 1 1
$renWin SetSize 500 500
$iren Initialize

set cam1 [$ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 ComputeViewPlaneNormal
$cam1 SetViewUp -0.16123 0.264271 0.950876

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

$renWin Render
#$renWin SetFileName "cut.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



