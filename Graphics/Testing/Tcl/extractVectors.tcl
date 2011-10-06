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
vtkMultiBlockPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
    set output [[pl3d GetOutput] GetBlock 0]

vtkExtractVectorComponents vx
  vx SetInputData $output
  vx Update

vtkContourFilter isoVx
    isoVx SetInputData [vx GetVxComponent]
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

vtkExtractVectorComponents vy
  vy SetInputData $output
  vy Update

vtkContourFilter isoVy
    isoVy SetInputData [vy GetVyComponent]
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

vtkExtractVectorComponents vz
  vz SetInputData $output
  vz Update

vtkContourFilter isoVz
    isoVz SetInputData [vz GetVzComponent]
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

vtkStructuredGridOutlineFilter outline
    outline SetInputData $output
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoVxActor
isoVxActor AddPosition 0 12 0
ren1 AddActor isoVyActor
ren1 AddActor isoVzActor
isoVzActor AddPosition 0 -12 0
ren1 SetBackground .8 .8 .8
renWin SetSize 321 321

[ren1 GetActiveCamera] SetPosition -63.3093 -1.55444 64.3922
[ren1 GetActiveCamera] SetFocalPoint 8.255 0.0499763 29.7631
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0 0 1
ren1 ResetCameraClippingRange

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
