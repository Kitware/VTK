package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderer ren2
vtkRenderWindow renWin
    renWin AddRenderer ren1
    renWin AddRenderer ren2
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline for ren1
#
vtkPLOT3DReader pl3d2
    pl3d2 SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d2 SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d2 SetScalarFunctionNumber 153
    pl3d2 Update

vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 120
    pl3d SetVectorFunctionNumber 202
    pl3d Update
vtkContourFilter iso
    iso SetInputConnection [pl3d GetOutputPort]
    iso SetValue 0 -100000

vtkProbeFilter probe2
  probe2 SetInputConnection [iso GetOutputPort]
  probe2 SetSource [pl3d2 GetOutput]

vtkCastToConcrete cast2 
  cast2 SetInputConnection [probe2 GetOutputPort]

vtkPolyDataNormals normals
    normals SetInput [cast2 GetPolyDataOutput]
    normals SetFeatureAngle 45
vtkPolyDataMapper isoMapper
    isoMapper SetInputConnection [normals GetOutputPort]
    isoMapper ScalarVisibilityOn
eval isoMapper SetScalarRange [[[[pl3d2 GetOutput] GetPointData] GetScalars] GetRange]

vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $bisque

vtkStructuredGridOutlineFilter outline
    outline SetInputConnection [pl3d GetOutputPort]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
ren1 SetViewport 0 0 .5 1

renWin SetSize 512 256
ren1 SetBackground 0.1 0.2 0.4

ren1 ResetCamera
set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 SetViewUp -0.16123 0.264271 0.950876


vtkPlaneSource aPlane
vtkPolyDataMapper aPlaneMapper
  aPlaneMapper SetInputConnection [aPlane GetOutputPort]
  aPlaneMapper ImmediateModeRenderingOn

vtkActor screen
  screen SetMapper aPlaneMapper

ren2 AddActor screen
ren2 SetViewport .5 0 1 1
[ren2 GetActiveCamera] Azimuth 30
[ren2 GetActiveCamera] Elevation 30
ren2 SetBackground .8 .4 .3
ren1 ResetCameraClippingRange
ren2 ResetCamera
ren2 ResetCameraClippingRange
renWin Render

vtkRendererSource ren1Image
  ren1Image SetInput ren1
  ren1Image DepthValuesOn
vtkTexture aTexture
  aTexture SetInputConnection [ren1Image GetOutputPort]

screen SetTexture aTexture

renWin Render

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


