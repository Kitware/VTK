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
  extractTensor SetInput [ptLoad GetOutput]
  extractTensor ScalarIsEffectiveStress
  extractTensor ScalarIsComponent
  extractTensor ExtractScalarsOn
  extractTensor ExtractVectorsOn
  extractTensor ExtractNormalsOff
  extractTensor ExtractTCoordsOn

vtkContourFilter contour
  contour SetInput [extractTensor GetOutput]
  contour SetValue 0 0

vtkProbeFilter probe
  probe SetInput [contour GetOutput]
  probe SetSource [ptLoad GetOutput]

vtkLoopSubdivisionFilter su
  su SetInput [probe GetOutput]
  su SetNumberOfSubdivisions 1

vtkPolyDataMapper s1Mapper
    s1Mapper SetInput [probe GetOutput]
#    s1Mapper SetInput [su GetOutput]

vtkActor s1Actor
    s1Actor SetMapper s1Mapper

#
# plane for context
#
vtkImageDataGeometryFilter g
    g SetInput [ptLoad GetOutput]
    g SetExtent 0 100 0 100 0 0
    g Update;#for scalar range

vtkPolyDataMapper gm
    gm SetInput [g GetOutput]
    eval gm SetScalarRange [[g GetOutput] GetScalarRange]
vtkActor ga
    ga SetMapper gm

    eval s1Mapper SetScalarRange [[g GetOutput] GetScalarRange]
#
# Create outline around data
#
vtkOutlineFilter outline
    outline SetInput [ptLoad GetOutput]
vtkPolyDataMapper outlineMapper
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
