package require vtk
package require vtkinteraction

# create tensor ellipsoids

# Create the RenderWindow, Renderer and interactive renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkPointLoad ptLoad
    ptLoad SetLoadValue 100.0
    ptLoad SetSampleDimensions 30 30 30
    ptLoad ComputeEffectiveStressOn
    ptLoad SetModelBounds -10 10 -10 10 -10 10

vtkExtractTensorComponents extractTensor
  extractTensor SetInputConnection [ptLoad GetOutputPort]
  extractTensor ScalarIsEffectiveStress
  extractTensor ScalarIsComponent
  extractTensor ExtractScalarsOn
  extractTensor ExtractVectorsOn
  extractTensor ExtractNormalsOff
  extractTensor ExtractTCoordsOn

vtkContourFilter contour
  contour SetInputConnection [extractTensor GetOutputPort]
  contour SetValue 0 0

vtkProbeFilter probe
  probe SetInputConnection [contour GetOutputPort]
  probe SetSource [ptLoad GetOutput]

vtkLoopSubdivisionFilter su
  su SetInputConnection [probe GetOutputPort]
  su SetNumberOfSubdivisions 1

vtkPolyDataMapper s1Mapper
    s1Mapper SetInputConnection [probe GetOutputPort]
#    s1Mapper SetInputConnection [su GetOutputPort]

vtkActor s1Actor
    s1Actor SetMapper s1Mapper

#
# plane for context
#
vtkImageDataGeometryFilter g
    g SetInputConnection [ptLoad GetOutputPort]
    g SetExtent 0 100 0 100 0 0
    g Update;#for scalar range

vtkPolyDataMapper gm
    gm SetInputConnection [g GetOutputPort]
    eval gm SetScalarRange [[g GetOutput] GetScalarRange]
vtkActor ga
    ga SetMapper gm

    eval s1Mapper SetScalarRange [[g GetOutput] GetScalarRange]
#
# Create outline around data
#
vtkOutlineFilter outline
    outline SetInputConnection [ptLoad GetOutputPort]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 0 0 0
#
# Create cone indicating application of load
#
vtkConeSource coneSrc
    coneSrc  SetRadius .5
    coneSrc  SetHeight 2
vtkPolyDataMapper coneMap
    coneMap SetInputConnection [coneSrc GetOutputPort]
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
    camera SetClippingRange 1 100

ren1 AddActor s1Actor
ren1 AddActor outlineActor
ren1 AddActor coneActor
ren1 AddActor ga
ren1 SetBackground 1.0 1.0 1.0
ren1 SetActiveCamera camera

renWin SetSize 300 300
renWin Render
iren AddObserver UserEvent {wm deiconify .vtkInteract}


# prevent the tk window from showing up then start the event loop
wm withdraw . 
