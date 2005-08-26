package require vtk

# create reader and warp data with vectors
vtkDataSetReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/blow.vtk"
    reader SetScalarsName "thickness9"
    reader SetVectorsName "displacement9"
vtkCastToConcrete castToUnstructuredGrid
    castToUnstructuredGrid SetInputConnection [reader GetOutputPort]
vtkWarpVector warp
    warp SetInput [castToUnstructuredGrid GetUnstructuredGridOutput]

# extract mold from mesh using connectivity
vtkConnectivityFilter connect
    connect SetInputConnection [warp GetOutputPort]
    connect SetExtractionModeToSpecifiedRegions
    connect AddSpecifiedRegion 0
    connect AddSpecifiedRegion 1
vtkDataSetMapper moldMapper
    moldMapper SetInputConnection [reader GetOutputPort]
    moldMapper ScalarVisibilityOff
vtkActor moldActor
    moldActor SetMapper moldMapper
    [moldActor GetProperty] SetColor .2 .2 .2
    [moldActor GetProperty] SetRepresentationToWireframe

# extract parison from mesh using connectivity
vtkConnectivityFilter connect2
    connect2 SetInputConnection [warp GetOutputPort]
    connect2 SetExtractionModeToSpecifiedRegions
    connect2 AddSpecifiedRegion 2
vtkExtractUnstructuredGrid extractGrid
    extractGrid SetInputConnection [connect2 GetOutputPort]
    extractGrid CellClippingOn
    extractGrid SetCellMinimum 0
    extractGrid SetCellMaximum 23
vtkGeometryFilter parison
    parison SetInputConnection [extractGrid GetOutputPort]
vtkPolyDataNormals normals2
    normals2 SetInputConnection [parison GetOutputPort]
    normals2 SetFeatureAngle 60
vtkLookupTable lut
    lut SetHueRange 0.0 0.66667
vtkPolyDataMapper parisonMapper
    parisonMapper SetInputConnection [normals2 GetOutputPort]
    parisonMapper SetLookupTable lut
    parisonMapper SetScalarRange 0.12 1.0
vtkActor parisonActor
    parisonActor SetMapper parisonMapper

# graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor parisonActor
ren1 AddActor moldActor
ren1 SetBackground 1 1 1
ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 60
[ren1 GetActiveCamera] Roll -90
[ren1 GetActiveCamera] Dolly 2
ren1 ResetCameraClippingRange
renWin SetSize 500 375
iren Initialize


# prevent the tk window from showing up then start the event loop
wm withdraw .
