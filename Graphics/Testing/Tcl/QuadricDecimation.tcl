package require vtk
package require vtkinteraction

# pipeline stuff
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
vtkGeometryFilter gf
    gf SetInputConnection [pl3d GetOutputPort]
vtkTriangleFilter tf
    tf SetInputConnection [gf GetOutputPort]
vtkPolyDataMapper gMapper
    gMapper SetInputConnection [gf GetOutputPort]
    eval gMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor gActor
    gActor SetMapper gMapper

# Don't look at attributes
vtkQuadricDecimation mesh
  mesh SetInputConnection [tf GetOutputPort]
  mesh SetTargetReduction .90
  mesh AttributeErrorMetricOn

vtkPolyDataMapper mapper
  mapper SetInputConnection [mesh GetOutputPort]

vtkActor actor
  actor SetMapper mapper

# This time worry about attributes
vtkQuadricDecimation mesh2
  mesh2 SetInputConnection [tf GetOutputPort]
  mesh2 SetTargetReduction .90
  mesh2 AttributeErrorMetricOff

vtkPolyDataMapper mapper2
  mapper2 SetInputConnection [mesh2 GetOutputPort]

vtkActor actor2
  actor2 SetMapper mapper2
  actor2 AddPosition 0 12 0

# Create rendering instances
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Set up the camera parameters
#
vtkCamera camera
  camera SetPosition 19.34 6.128 -11.96
  camera SetFocalPoint 8.25451 6.0 29.77
  camera SetViewUp 0.9664 0.00605 0.256883
  camera SetViewAngle 30
  camera SetClippingRange 26 64

ren1 SetActiveCamera camera

# Add the actors to the renderer, set the background and size
#
ren1 AddActor actor
ren1 AddActor actor2

ren1 SetBackground 1 1 1
renWin SetSize 400 400
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

set threshold 50
