package require vtk
package require vtkinteraction
package require vtktesting

# create pipeline
# reader reads slices
vtkVolume16Reader v16
    v16 SetDataDimensions 64 64
    v16 SetDataByteOrderToLittleEndian
    v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
    v16 SetDataSpacing 3.2 3.2 1.5
    v16 SetImageRange 30 50
    v16 SetDataMask 0x7fff

# create points on edges
vtkEdgePoints edgePoints
  edgePoints SetInputConnection [v16 GetOutputPort]
  edgePoints SetValue 1150

#
vtkDataSetMapper mapper
  mapper SetInputConnection [edgePoints GetOutputPort]
  mapper ImmediateModeRenderingOn
  mapper ScalarVisibilityOff
   
vtkActor head
    head SetMapper mapper
    eval [head GetProperty] SetColor $raw_sienna

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor head
ren1 SetBackground 1 1 1
renWin SetSize 300 300
eval ren1 SetBackground $slate_grey
[ren1 GetActiveCamera] SetPosition 99.8847 537.86 22.4716 
[ren1 GetActiveCamera] SetFocalPoint 99.8847 109.81 15 
[ren1 GetActiveCamera] SetViewAngle 20
[ren1 GetActiveCamera] SetViewUp 0 -1 0

ren1 ResetCameraClippingRange

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .
