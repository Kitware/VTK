package require vtk
package require vtkinteraction
package require vtktesting

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkVolume16Reader v16
  v16 SetDataDimensions 64 64
  [v16 GetOutput] SetOrigin 0.0 0.0 0.0
  v16 SetDataByteOrderToLittleEndian
  v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  v16 SetImageRange 1 93
  v16 SetDataSpacing 3.2 3.2 1.5
  v16 Update

vtkMergePoints myLocator

vtkMarchingSquares isoXY
  isoXY SetInput [v16 GetOutput]
  isoXY GenerateValues 2 600 1200
  isoXY SetImageRange 0 32 32 63 45 45
  isoXY SetLocator myLocator

vtkPolyDataMapper isoXYMapper
  isoXYMapper SetInput [isoXY GetOutput]
  isoXYMapper SetScalarRange 600 1200

vtkActor isoXYActor
  isoXYActor SetMapper isoXYMapper

vtkMarchingSquares isoYZ
  isoYZ SetInput [v16 GetOutput]
  isoYZ GenerateValues 2 600 1200
  isoYZ SetImageRange 32 32 32 63 46 92

vtkPolyDataMapper isoYZMapper
  isoYZMapper SetInput [isoYZ GetOutput]
  isoYZMapper SetScalarRange 600 1200

vtkActor isoYZActor
  isoYZActor SetMapper isoYZMapper

vtkMarchingSquares isoXZ
  isoXZ SetInput [v16 GetOutput]
  isoXZ GenerateValues 2 600 1200
  isoXZ SetImageRange 0 32 32 32 0 46

vtkPolyDataMapper isoXZMapper
  isoXZMapper SetInput [isoXZ GetOutput]
  isoXZMapper SetScalarRange 600 1200

vtkActor isoXZActor
  isoXZActor SetMapper isoXZMapper

vtkOutlineFilter outline
  outline SetInput [v16 GetOutput]
vtkPolyDataMapper outlineMapper
  outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
  outlineActor SetMapper outlineMapper
  outlineActor VisibilityOff

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoXYActor
ren1 AddActor isoYZActor
ren1 AddActor isoXZActor
ren1 SetBackground 0.9 .9 .9
renWin SetSize 200 200
[ren1 GetActiveCamera] SetPosition 324.368 284.266 -19.3293 
[ren1 GetActiveCamera] SetFocalPoint 73.5683 120.903 70.7309 
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp -0.304692 -0.0563843 -0.950781 
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


