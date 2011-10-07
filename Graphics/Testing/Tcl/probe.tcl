package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetMultiSamples 0
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# cut data
vtkMultiBlockPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
    set output [[pl3d GetOutput] GetBlock 0]
vtkPlane plane
    eval plane SetOrigin [$output GetCenter]
    plane SetNormal -0.287 0 0.9579
vtkCutter planeCut
    planeCut SetInputData $output
    planeCut SetCutFunction plane
vtkProbeFilter probe
    probe SetInputConnection [planeCut GetOutputPort]
    probe SetSourceData $output
vtkDataSetMapper cutMapper
    cutMapper SetInputConnection [probe GetOutputPort]
    eval cutMapper SetScalarRange \
      [[[$output GetPointData] GetScalars] GetRange]
vtkActor cutActor
    cutActor SetMapper cutMapper

#extract plane
vtkStructuredGridGeometryFilter compPlane
    compPlane SetInputData $output
    compPlane SetExtent 0 100 0 100 9 9
vtkPolyDataMapper planeMapper
    planeMapper SetInputConnection [compPlane GetOutputPort]
    planeMapper ScalarVisibilityOff
vtkActor planeActor
    planeActor SetMapper planeMapper
    [planeActor GetProperty] SetRepresentationToWireframe
    [planeActor GetProperty] SetColor 0 0 0

#outline
vtkStructuredGridOutlineFilter outline
    outline SetInputData $output
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor planeActor
ren1 AddActor cutActor
ren1 SetBackground 1 1 1
renWin SetSize 400 300

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 11.1034 59.5328
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition -2.95748 -26.7271 44.5309
$cam1 SetViewUp 0.0184785 0.479657 0.877262
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



