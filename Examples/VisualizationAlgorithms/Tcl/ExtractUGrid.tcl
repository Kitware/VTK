# This example shows how to extract portions of an unstructured grid
# using vtkExtractUnstructuredGrid. vtkConnectivityFilter is also used
# to extract connected components.
#
# The data found here represents a blow molding process. Blow molding
# requires a mold and parison (hot, viscous plastic) which is shaped
# by the mold into the final form. The data file contains several steps
# in time for the analysis.
#

package require vtk

# Create a reader to read the unstructured grid data. We use a 
# vtkDataSetReader which means the type of the output is unknown until
# the data file is read. SO we follow the reader with a vtkCastToConcrete
# and cast the output to vtkUnstructuredGrid.
vtkDataSetReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/blow.vtk"
    reader SetScalarsName "thickness9"
    reader SetVectorsName "displacement9"
vtkCastToConcrete castToUnstructuredGrid
    castToUnstructuredGrid SetInput [reader GetOutput]
vtkWarpVector warp
    warp SetInput [castToUnstructuredGrid GetUnstructuredGridOutput]

# The connectivity filter extracts the first two regions. These are
# know to represent the mold.
vtkConnectivityFilter connect
    connect SetInput [warp GetOutput]
    connect SetExtractionModeToSpecifiedRegions
    connect AddSpecifiedRegion 0
    connect AddSpecifiedRegion 1
vtkDataSetMapper moldMapper
    moldMapper SetInput [reader GetOutput]
    moldMapper ScalarVisibilityOff
vtkActor moldActor
    moldActor SetMapper moldMapper
    [moldActor GetProperty] SetColor .2 .2 .2
    [moldActor GetProperty] SetRepresentationToWireframe

# Another connectivity filter is used to extract the parison.
vtkConnectivityFilter connect2
    connect2 SetInput [warp GetOutput]
    connect2 SetExtractionModeToSpecifiedRegions
    connect2 AddSpecifiedRegion 2
# We use vtkExtractUnstructuredGrid because we are interested in
# looking at just a few cells. We use cell clipping via cell id to
# extract the portion of the grid we are interested in.
vtkExtractUnstructuredGrid extractGrid
    extractGrid SetInput [connect2 GetOutput]
    extractGrid CellClippingOn
    extractGrid SetCellMinimum 0
    extractGrid SetCellMaximum 23
vtkGeometryFilter parison
    parison SetInput [extractGrid GetOutput]
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
[ren1 GetActiveCamera] Azimuth 60
[ren1 GetActiveCamera] Roll -90
[ren1 GetActiveCamera] Dolly 2
ren1 ResetCameraClippingRange
renWin SetSize 500 375
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .
