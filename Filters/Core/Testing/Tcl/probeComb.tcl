package require vtk
package require vtkinteraction

# create planes
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin SetMultiSamples 0
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

vtkPlaneSource plane
    plane SetResolution 50 50
vtkTransform transP1
    transP1 Translate 3.7 0.0 28.37
    transP1 Scale 5 5 5
    transP1 RotateY 90
vtkTransformPolyDataFilter tpd1
    tpd1 SetInputConnection [plane GetOutputPort]
    tpd1 SetTransform transP1
vtkOutlineFilter outTpd1
    outTpd1 SetInputConnection [tpd1 GetOutputPort]
vtkPolyDataMapper mapTpd1
    mapTpd1 SetInputConnection [outTpd1 GetOutputPort]
vtkActor tpd1Actor
    tpd1Actor SetMapper mapTpd1
    [tpd1Actor GetProperty] SetColor 0 0 0

vtkTransform transP2
    transP2 Translate 9.2 0.0 31.20
    transP2 Scale 5 5 5
    transP2 RotateY 90
vtkTransformPolyDataFilter tpd2
    tpd2 SetInputConnection [plane GetOutputPort]
    tpd2 SetTransform transP2
vtkOutlineFilter outTpd2
    outTpd2 SetInputConnection [tpd2 GetOutputPort]
vtkPolyDataMapper mapTpd2
    mapTpd2 SetInputConnection [outTpd2 GetOutputPort]
vtkActor tpd2Actor
    tpd2Actor SetMapper mapTpd2
    [tpd2Actor GetProperty] SetColor 0 0 0

vtkTransform transP3
    transP3 Translate 13.27 0.0 33.30
    transP3 Scale 5 5 5
    transP3 RotateY 90
vtkTransformPolyDataFilter tpd3
    tpd3 SetInputConnection [plane GetOutputPort]
    tpd3 SetTransform transP3
vtkOutlineFilter outTpd3
    outTpd3 SetInputConnection [tpd3 GetOutputPort]
vtkPolyDataMapper mapTpd3
    mapTpd3 SetInputConnection [outTpd3 GetOutputPort]
vtkActor tpd3Actor
    tpd3Actor SetMapper mapTpd3
    [tpd3Actor GetProperty] SetColor 0 0 0

vtkAppendPolyData appendF
    appendF AddInputConnection [tpd1 GetOutputPort]
    appendF AddInputConnection [tpd2 GetOutputPort]
    appendF AddInputConnection [tpd3 GetOutputPort]

vtkProbeFilter probe
    probe SetInputConnection [appendF GetOutputPort]
    probe SetSourceData $output

vtkContourFilter contour
    contour SetInputConnection [probe GetOutputPort]
    eval contour GenerateValues 50 [$output GetScalarRange]

vtkPolyDataMapper contourMapper
    contourMapper SetInputConnection [contour GetOutputPort]
    eval contourMapper SetScalarRange [$output GetScalarRange]
vtkActor planeActor
    planeActor SetMapper contourMapper

vtkStructuredGridOutlineFilter outline
    outline SetInputData $output
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0

ren1 AddActor outlineActor
ren1 AddActor planeActor
ren1 AddActor tpd1Actor
ren1 AddActor tpd2Actor
ren1 AddActor tpd3Actor
ren1 SetBackground 1 1 1
renWin SetSize 400 400

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 8.88908 0.595038 29.3342
$cam1 SetPosition -12.3332 31.7479 41.2387
$cam1 SetViewUp 0.060772 -0.319905 0.945498
iren Initialize


# prevent the tk window from showing up then start the event loop
wm withdraw .



