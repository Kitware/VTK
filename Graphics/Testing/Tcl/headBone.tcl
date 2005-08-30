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
vtkLight lgt

# create pipeline
#
vtkMergePoints locator
  locator SetDivisions 32 32 46
  locator RetainCellListsOff
  locator SetNumberOfPointsPerBucket 2
  locator AutomaticOff

vtkVolume16Reader v16
    v16 SetDataDimensions 64 64
    [v16 GetOutput] SetOrigin 0.0 0.0 0.0
    v16 SetDataByteOrderToLittleEndian
    v16 SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
    v16 SetImageRange 1 93
    v16 SetDataSpacing 3.2 3.2 1.5

vtkMarchingCubes iso
    iso SetInputConnection [v16 GetOutputPort]
    iso SetValue 0 1150
    iso ComputeGradientsOn
    iso ComputeScalarsOff
    iso SetLocator locator

vtkVectorNorm gradient
  gradient SetInputConnection [iso GetOutputPort]

vtkDataSetMapper isoMapper
    isoMapper SetInputConnection [gradient GetOutputPort]
    isoMapper ScalarVisibilityOn
    isoMapper SetScalarRange 0 1200
    isoMapper ImmediateModeRenderingOn

vtkActor isoActor
    isoActor SetMapper isoMapper
set isoProp [isoActor GetProperty]
eval $isoProp SetColor $antique_white

vtkOutlineFilter outline
    outline SetInputConnection [v16 GetOutputPort]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
#eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
ren1 AddLight lgt
renWin SetSize 250 250
ren1 SetBackground 0.1 0.2 0.4

ren1 ResetCamera
set cam1 [ren1 GetActiveCamera]
$cam1 Elevation 90
$cam1 SetViewUp 0 0 -1
$cam1 Zoom 1.5
eval lgt SetPosition [$cam1 GetPosition]
eval lgt SetFocalPoint [$cam1 GetFocalPoint]

ren1 ResetCameraClippingRange

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .


