# Create blow molding image (data point 5)
# get the interactor
source ../../examplesTcl/vtkInt.tcl

# create reader and warp data with vectors
vtkUnstructuredGridReader reader
    reader SetFileName "../../../vtkdata/blow.vtk"
    reader SetScalarsName "thickness9"
    reader SetVectorsName "displacement9"
vtkWarpVector warp
    warp SetInput [reader GetOutput]

# extract mold from mesh using connectivity
vtkConnectivityFilter connect
    connect SetInput [warp GetOutput]
    connect SetExtractionModeToSpecifiedRegions
    connect AddSpecifiedRegion 0
    connect AddSpecifiedRegion 1
vtkDataSetMapper moldMapper
    moldMapper SetInput [connect GetOutput]
    moldMapper ScalarVisibilityOff
vtkActor moldActor
    moldActor SetMapper moldMapper
    [moldActor GetProperty] SetColor .2 .2 .2
    [moldActor GetProperty] SetRepresentationToWireframe

# extract parison from mesh using connectivity
vtkConnectivityFilter connect2
    connect2 SetInput [warp GetOutput]
    connect2 SetExtractionModeToSpecifiedRegions
    connect2 AddSpecifiedRegion 2
vtkGeometryFilter parison
    parison SetInput [connect2 GetOutput]
vtkPolyDataNormals normals2
    normals2 SetInput [parison GetOutput]
    normals2 SetFeatureAngle 60
vtkLookupTable lut
    lut SetHueRange 0.0 0.66667
vtkPolyDataMapper parisonMapper
    parisonMapper SetInput [normals2 GetOutput]
    parisonMapper SetLookupTable lut
    parisonMapper SetScalarRange 0.12 1.0
vtkActor parisonActor
    parisonActor SetMapper parisonMapper

# Create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
ren1 AddActor moldActor
ren1 AddActor parisonActor
ren1 SetBackground 1 1 1
renWin SetSize 500 350
[ren1 GetActiveCamera] Roll 90
[ren1 GetActiveCamera] Elevation 90
[ren1 GetActiveCamera] Zoom 2
iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName blow.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
