# Test the scalar bar actor using a logarithmic lookup table
#
package require vtk
package require vtkinteraction

set VTK_INTEGRATE_BOTH_DIRECTIONS 2

#
# generate tensors
vtkPointLoad ptLoad
    ptLoad SetLoadValue 100.0
    ptLoad SetSampleDimensions 20 20 20
    ptLoad ComputeEffectiveStressOn
    ptLoad SetModelBounds -10 10 -10 10 -10 10

# Generate hyperstreamlines
vtkHyperStreamline s1
    s1 SetInput [ptLoad GetOutput]
    s1 SetStartPosition 9 9 -9
    s1 IntegrateMinorEigenvector
    s1 SetMaximumPropagationDistance 18.0
    s1 SetIntegrationStepLength 0.1
    s1 SetStepLength 0.01
    s1 SetRadius 0.25
    s1 SetNumberOfSides 18
    s1 SetIntegrationDirection $VTK_INTEGRATE_BOTH_DIRECTIONS
    s1 Update

# Map hyperstreamlines
vtkLogLookupTable lut
    lut SetHueRange .6667 0.0
vtkScalarBarActor scalarBar
    scalarBar SetLookupTable lut
    scalarBar SetTitle "Stress"
    [scalarBar GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport
    [scalarBar GetPositionCoordinate] SetValue 0.1 0.05
    scalarBar SetOrientationToVertical
    scalarBar SetWidth 0.1
    scalarBar SetHeight 0.9
    scalarBar SetPosition 0.01 0.1
    scalarBar SetLabelFormat "%-#6.3f"
    [scalarBar GetLabelTextProperty] SetColor 1 0 0
    [scalarBar GetTitleTextProperty] SetColor 1 0 0
vtkPolyDataMapper s1Mapper
    s1Mapper SetInput [s1 GetOutput]
    s1Mapper SetLookupTable lut
    ptLoad Update;#force update for scalar range
    eval s1Mapper SetScalarRange [[ptLoad GetOutput] GetScalarRange]
vtkActor s1Actor
    s1Actor SetMapper s1Mapper

vtkHyperStreamline s2
    s2 SetInput [ptLoad GetOutput]
    s2 SetStartPosition -9 -9 -9
    s2 IntegrateMinorEigenvector
    s2 SetMaximumPropagationDistance 18.0
    s2 SetIntegrationStepLength 0.1
    s2 SetStepLength 0.01
    s2 SetRadius 0.25
    s2 SetNumberOfSides 18
    s2 SetIntegrationDirection $VTK_INTEGRATE_BOTH_DIRECTIONS
    s2 Update
vtkPolyDataMapper s2Mapper
    s2Mapper SetInput [s2 GetOutput]
    s2Mapper SetLookupTable lut
    eval s2Mapper SetScalarRange [[ptLoad GetOutput] GetScalarRange]
vtkActor s2Actor
    s2Actor SetMapper s2Mapper

vtkHyperStreamline s3
    s3 SetInput [ptLoad GetOutput]
    s3 SetStartPosition 9 -9 -9
    s3 IntegrateMinorEigenvector
    s3 SetMaximumPropagationDistance 18.0
    s3 SetIntegrationStepLength 0.1
    s3 SetStepLength 0.01
    s3 SetRadius 0.25
    s3 SetNumberOfSides 18
    s3 SetIntegrationDirection $VTK_INTEGRATE_BOTH_DIRECTIONS
    s3 Update
vtkPolyDataMapper s3Mapper
    s3Mapper SetInput [s3 GetOutput]
    s3Mapper SetLookupTable lut
    eval s3Mapper SetScalarRange [[ptLoad GetOutput] GetScalarRange]
vtkActor s3Actor
    s3Actor SetMapper s3Mapper

vtkHyperStreamline s4
    s4 SetInput [ptLoad GetOutput]
    s4 SetStartPosition -9 9 -9
    s4 IntegrateMinorEigenvector
    s4 SetMaximumPropagationDistance 18.0
    s4 SetIntegrationStepLength 0.1
    s4 SetStepLength 0.01
    s4 SetRadius 0.25
    s4 SetNumberOfSides 18
    s4 SetIntegrationDirection $VTK_INTEGRATE_BOTH_DIRECTIONS
    s4 Update
vtkPolyDataMapper s4Mapper
    s4Mapper SetInput [s4 GetOutput]
    s4Mapper SetLookupTable lut
    eval s4Mapper SetScalarRange [[ptLoad GetOutput] GetScalarRange]
vtkActor s4Actor
    s4Actor SetMapper s4Mapper

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

# Create outline around data
#
vtkOutlineFilter outline
    outline SetInput [ptLoad GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 0 0 0

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

# Create the rendering infrastructure
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkCamera camera
    camera SetFocalPoint 0.113766 -1.13665 -1.01919
    camera SetPosition -29.4886 -63.1488 26.5807
    camera SetViewAngle 24.4617
    camera SetViewUp 0.17138 0.331163 0.927879
    camera SetClippingRange 1 100

ren1 AddActor2D scalarBar
ren1 AddActor s1Actor
ren1 AddActor s2Actor
ren1 AddActor s3Actor
ren1 AddActor s4Actor
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
