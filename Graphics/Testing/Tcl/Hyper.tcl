package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and interactive renderer
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

set VTK_INTEGRATE_BOTH_DIRECTIONS 2

#
# generate tensors
vtkPointLoad ptLoad
    ptLoad SetLoadValue 100.0
    ptLoad SetSampleDimensions 20 20 20
    ptLoad ComputeEffectiveStressOn
    ptLoad SetModelBounds -10 10 -10 10 -10 10



#
# If the current directory is writable, then test the witers
#
if {[catch {set channel [open "test.tmp" "w"]}] == 0 } {
   close $channel
   file delete -force "test.tmp"

   vtkDataSetWriter wSP
   wSP SetInputConnection [ptLoad GetOutputPort]
   wSP SetFileName "wSP.vtk"
   wSP SetTensorsName "pointload"
   wSP SetScalarsName "effective_stress"
   wSP Write

   vtkDataSetReader rSP
   rSP SetFileName "wSP.vtk"
   rSP SetTensorsName "pointload"
   rSP SetScalarsName "effective_stress"
   rSP Update
   
   set input [rSP GetOutput]
   
   file delete -force "wSP.vtk"
} else {
   set input [ptLoad GetOutput]
}

# Generate hyperstreamlines
vtkHyperStreamline s1
s1 SetInput $input
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
vtkPolyDataMapper s1Mapper
    s1Mapper SetInputConnection [s1 GetOutputPort]
    s1Mapper SetLookupTable lut
    ptLoad Update;#force update for scalar range
    eval s1Mapper SetScalarRange [[ptLoad GetOutput] GetScalarRange]
vtkActor s1Actor
    s1Actor SetMapper s1Mapper

vtkHyperStreamline s2
    s2 SetInput $input
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
    s2Mapper SetInputConnection [s2 GetOutputPort]
    s2Mapper SetLookupTable lut
    eval s2Mapper SetScalarRange [$input GetScalarRange]
vtkActor s2Actor
    s2Actor SetMapper s2Mapper

vtkHyperStreamline s3
    s3 SetInput $input
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
    s3Mapper SetInputConnection [s3 GetOutputPort]
    s3Mapper SetLookupTable lut
    eval s3Mapper SetScalarRange [$input GetScalarRange]
vtkActor s3Actor
    s3Actor SetMapper s3Mapper

vtkHyperStreamline s4
    s4 SetInput $input
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
    s4Mapper SetInputConnection [s4 GetOutputPort]
    s4Mapper SetLookupTable lut
    eval s4Mapper SetScalarRange [$input GetScalarRange]
vtkActor s4Actor
    s4Actor SetMapper s4Mapper

# plane for context
#
vtkImageDataGeometryFilter g
    g SetInput $input
    g SetExtent 0 100 0 100 0 0
    g Update;#for scalar range
vtkPolyDataMapper gm
    gm SetInputConnection [g GetOutputPort]
    eval gm SetScalarRange [[g GetOutput] GetScalarRange]
vtkActor ga
    ga SetMapper gm

# Create outline around data
#
vtkOutlineFilter outline
    outline SetInput $input
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 0 0 0

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
