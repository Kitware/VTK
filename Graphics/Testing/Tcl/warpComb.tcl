package require vtktcl

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
vtkStructuredGridGeometryFilter plane
    plane SetInput [pl3d GetOutput]
    plane SetExtent 10 10 1 100 1 100
vtkStructuredGridGeometryFilter plane2
    plane2 SetInput [pl3d GetOutput]
    plane2 SetExtent 30 30 1 100 1 100
vtkStructuredGridGeometryFilter plane3
    plane3 SetInput [pl3d GetOutput]
    plane3 SetExtent 45 45 1 100 1 100
vtkAppendPolyData appendF
    appendF AddInput [plane GetOutput]
    appendF AddInput [plane2 GetOutput]
    appendF AddInput [plane3 GetOutput]
vtkWarpScalar warp
    warp SetInput [appendF GetOutput]
    warp UseNormalOn
    warp SetNormal 1.0 0.0 0.0
    warp SetScaleFactor 2.5
vtkPolyDataNormals normals
    normals SetInput [warp GetPolyDataOutput]
    normals SetFeatureAngle 60
vtkPolyDataMapper planeMapper
    planeMapper SetInput [normals GetOutput]
    eval planeMapper SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor planeActor
    planeActor SetMapper planeMapper

vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0

ren1 AddActor outlineActor
ren1 AddActor planeActor
ren1 SetBackground 1 1 1
renWin SetSize 300 300

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 8.88908 0.595038 29.3342
$cam1 SetPosition -12.3332 31.7479 41.2387
$cam1 SetViewUp 0.060772 -0.319905 0.945498
iren Initialize

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .



