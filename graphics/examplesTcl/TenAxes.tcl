catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# create tensor ellipsoids
# Create the RenderWindow, Renderer and interactive renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

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
vtkAxes axes
    axes SetScaleFactor 0.5
vtkTubeFilter tubeAxes
    tubeAxes SetInput [axes GetOutput]
    tubeAxes SetRadius 0.1
    tubeAxes SetNumberOfSides 6
vtkTensorGlyph ellipsoids
    ellipsoids SetInput [ptLoad GetOutput]
    ellipsoids SetSource [axes GetOutput]
    ellipsoids SetScaleFactor 10
    ellipsoids ClampScalingOn
  
# Map contour
vtkLogLookupTable lut
    lut SetHueRange .6667 0.0
vtkPolyDataMapper ellipMapper
    ellipMapper SetInput [ellipsoids GetOutput]
    ellipMapper SetLookupTable lut
    ellipMapper ImmediateModeRenderingOn
    plane Update;#force update for scalar range
    eval ellipMapper SetScalarRange [[plane GetOutput] GetScalarRange]

vtkActor ellipActor
    ellipActor SetMapper ellipMapper
    [ellipActor GetProperty] SetAmbient 1
    [ellipActor GetProperty] SetDiffuse 0
#
# Create outline around data
#
vtkOutlineFilter outline
    outline SetInput [ptLoad GetOutput]

vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]

vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0

#
# Create cone indicating application of load
#
vtkConeSource coneSrc
    coneSrc  SetRadius .5
    coneSrc  SetHeight 2
vtkPolyDataMapper coneMap
    coneMap SetInput [coneSrc GetOutput]
vtkActor coneActor
    coneActor SetMapper coneMap;    
    coneActor SetPosition 0 0 11
    coneActor RotateY 90
    eval [coneActor GetProperty] SetColor 1 0 0

vtkCamera camera
    camera SetFocalPoint 0.113766 -1.13665 -1.01919
    camera SetPosition -29.4886 -63.1488 26.5807
    camera SetViewAngle 24.4617
    camera SetViewUp 0.17138 0.331163 0.927879

ren1 AddActor ellipActor
ren1 AddActor outlineActor
ren1 AddActor coneActor
ren1 SetBackground 1.0 1.0 1.0
ren1 SetActiveCamera camera

renWin SetSize 500 500
renWin Render

#renWin SetFileName TenAxes.tcl.ppm
#renWin SaveImageAsPPM

iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .




