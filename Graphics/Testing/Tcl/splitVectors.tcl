package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

vtkSplitField sf
    sf SetInputConnection [pl3d GetOutputPort]
    sf SetInputField "VECTORS" "POINT_DATA"
    sf Split 0 "vx"
    sf Split 1 "vy"
    sf Split 2 "vz"

sf Print

vtkAssignAttribute aax
   aax SetInputConnection [sf GetOutputPort]
   aax Assign "vx" "SCALARS" "POINT_DATA"
vtkContourFilter isoVx
    isoVx SetInputConnection [aax GetOutputPort]
    isoVx SetValue 0 .38
vtkPolyDataNormals normalsVx
    normalsVx SetInputConnection [isoVx GetOutputPort]
    normalsVx SetFeatureAngle 45
vtkPolyDataMapper isoVxMapper
    isoVxMapper SetInputConnection [normalsVx GetOutputPort]
    isoVxMapper ScalarVisibilityOff
    isoVxMapper ImmediateModeRenderingOn
vtkActor isoVxActor
    isoVxActor SetMapper isoVxMapper
    eval [isoVxActor GetProperty] SetColor 1 0.7 0.6

vtkAssignAttribute aay
   aay SetInputConnection [sf GetOutputPort]
   aay Assign "vy" "SCALARS" "POINT_DATA"
vtkContourFilter isoVy
    isoVy SetInputConnection [aay GetOutputPort]
    isoVy SetValue 0 .38
vtkPolyDataNormals normalsVy
    normalsVy SetInputConnection [isoVy GetOutputPort]
    normalsVy SetFeatureAngle 45
vtkPolyDataMapper isoVyMapper
    isoVyMapper SetInputConnection [normalsVy GetOutputPort]
    isoVyMapper ScalarVisibilityOff
    isoVyMapper ImmediateModeRenderingOn
vtkActor isoVyActor
    isoVyActor SetMapper isoVyMapper
    eval [isoVyActor GetProperty] SetColor 0.7 1 0.6

vtkAssignAttribute aaz
   aaz SetInputConnection [sf GetOutputPort]
   aaz Assign "vz" "SCALARS" "POINT_DATA"
vtkContourFilter isoVz
    isoVz SetInputConnection [aaz GetOutputPort]
    isoVz SetValue 0 .38
vtkPolyDataNormals normalsVz
    normalsVz SetInputConnection [isoVz GetOutputPort]
    normalsVz SetFeatureAngle 45
vtkPolyDataMapper isoVzMapper
    isoVzMapper SetInputConnection [normalsVz GetOutputPort]
    isoVzMapper ScalarVisibilityOff
    isoVzMapper ImmediateModeRenderingOn
vtkActor isoVzActor
    isoVzActor SetMapper isoVzMapper
    eval [isoVzActor GetProperty] SetColor 0.4 0.5 1

vtkMergeFields mf
   mf SetInputConnection [sf GetOutputPort]
   mf SetOutputField "merged" "POINT_DATA"
   mf SetNumberOfComponents 3
   mf Merge 0 "vy" 0
   mf Merge 1 "vz" 0
   mf Merge 2 "vx" 0

mf Print

vtkAssignAttribute aa
   aa SetInputConnection [mf GetOutputPort]
   aa Assign "merged" "SCALARS" "POINT_DATA"
vtkAssignAttribute aa2
   aa2 SetInputConnection [aa GetOutputPort]
   aa2 Assign "SCALARS" "VECTORS" "POINT_DATA"
vtkStreamLine sl
   sl SetInputConnection [aa2 GetOutputPort]
   sl SetStartPosition 2 -2 26
   sl SetMaximumPropagationTime 40
   sl SetIntegrationStepLength 0.2
   sl SetIntegrationDirectionToForward
   sl SetStepLength 0.001
vtkRibbonFilter rf
   rf SetInputConnection [sl GetOutputPort]
   rf SetWidth 1.0
   rf SetWidthFactor 5
vtkPolyDataMapper slMapper
    slMapper SetInputConnection [rf GetOutputPort]
    slMapper ImmediateModeRenderingOn
vtkActor slActor
    slActor SetMapper slMapper

vtkStructuredGridOutlineFilter outline
    outline SetInputConnection [pl3d GetOutputPort]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor isoVxActor
isoVxActor AddPosition 0 12 0
ren1 AddActor isoVyActor
ren1 AddActor isoVzActor
isoVzActor AddPosition 0 -12 0
ren1 AddActor slActor
slActor AddPosition 0 24 0
ren1 AddActor outlineActor
outlineActor AddPosition 0 24 0
ren1 SetBackground .8 .8 .8
renWin SetSize 321 321

[ren1 GetActiveCamera] SetPosition -20.3093 20.55444 64.3922
[ren1 GetActiveCamera] SetFocalPoint 8.255 0.0499763 29.7631
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0 0 1
[ren1 GetActiveCamera] Dolly 0.4
ren1 ResetCameraClippingRange


# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


