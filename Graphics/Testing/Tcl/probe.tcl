package require vtk
package require vtkinteraction

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# cut data
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA_ROOT/Data/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA_ROOT/Data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
vtkPlane plane
    eval plane SetOrigin [[pl3d GetOutput] GetCenter]
    plane SetNormal -0.287 0 0.9579
vtkCutter planeCut
    planeCut SetInput [pl3d GetOutput]
    planeCut SetCutFunction plane
vtkProbeFilter probe
    probe SetInput [planeCut GetOutput]
    probe SetSource [pl3d GetOutput]
vtkDataSetMapper cutMapper
    cutMapper SetInput [probe GetOutput]
    eval cutMapper SetScalarRange \
      [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange]
vtkActor cutActor
    cutActor SetMapper cutMapper

#extract plane
vtkStructuredGridGeometryFilter compPlane
    compPlane SetInput [pl3d GetOutput]
    compPlane SetExtent 0 100 0 100 9 9
vtkPolyDataMapper planeMapper
    planeMapper SetInput [compPlane GetOutput]
    planeMapper ScalarVisibilityOff
vtkActor planeActor
    planeActor SetMapper planeMapper
    [planeActor GetProperty] SetRepresentationToWireframe
    [planeActor GetProperty] SetColor 0 0 0

#outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
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



