catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl

# create tensor ellipsoids
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and interactive renderer
#
set renWin [rm MakeRenderWindow]
set ren1 [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

#
# Create tensor ellipsoids
#
# generate tensors
vtkPointLoad ptLoad
    ptLoad SetLoadValue 100.0
    ptLoad SetSampleDimensions 6 6 6
    ptLoad ComputeEffectiveStressOn
    ptLoad SetModelBounds -10 10 -10 10 -10 10

# extract plane of data
vtkStructuredPointsGeometryFilter plane
    plane SetInput [ptLoad GetOutput]
    plane SetExtent 2 2 0 99 0 99

# Generate ellipsoids
vtkSphereSource sphere
    sphere SetThetaResolution 8
    sphere SetPhiResolution 8
vtkTensorGlyph ellipsoids
    ellipsoids SetInput [ptLoad GetOutput]
    ellipsoids SetSource [sphere GetOutput]
    ellipsoids SetScaleFactor 10
    ellipsoids ClampScalingOn
  
# Map contour
vtkLogLookupTable lut
    lut SetHueRange .6667 0.0
vtkPolyMapper ellipMapper
    ellipMapper SetInput [ellipsoids GetOutput]
    ellipMapper SetLookupTable lut
    plane Update;#force update for scalar range
    eval ellipMapper SetScalarRange [[plane GetOutput] GetScalarRange]

vtkActor ellipActor
    ellipActor SetMapper ellipMapper
#
# Create outline around data
#
vtkOutlineFilter outline
    outline SetInput [ptLoad GetOutput]

vtkPolyMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]

vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 0 0 0

#
# Create cone indicating application of load
#
vtkConeSource coneSrc
    coneSrc  SetRadius .5
    coneSrc  SetHeight 2
vtkPolyMapper coneMap
    coneMap SetInput [coneSrc GetOutput]
vtkActor coneActor
    coneActor SetMapper coneMap;    
    coneActor SetPosition 0 0 11
    coneActor RotateY 90
    eval [coneActor GetProperty] SetColor 1 0 0

vtkCamera camera
    camera SetFocalPoint 0.113766 -1.13665 -1.01919
    camera SetPosition -29.4886 -63.1488 26.5807
    camera ComputeViewPlaneNormal
    camera SetViewAngle 24.4617
    camera SetViewUp 0.17138 0.331163 0.927879

$ren1 AddActor ellipActor
$ren1 AddActor outlineActor
$ren1 AddActor coneActor
$ren1 SetBackground 1.0 1.0 1.0
$ren1 SetActiveCamera camera

$renWin SetSize 450 450
$renWin Render
$iren SetUserMethod {wm deiconify .vtkInteract}

#$renWin SetFileName TenEllip.tcl.ppm
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
