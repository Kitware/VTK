package require vtk
package require vtkinteraction

# Read a field representing unstructured grid and display it (similar to blow.tcl)

# create a reader and write out field daya
vtkUnstructuredGridReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/blow.vtk"
    reader SetScalarsName "thickness9"
    reader SetVectorsName "displacement9"
vtkDataSetToDataObjectFilter ds2do
    ds2do SetInput [reader GetOutput]

# we must be able to write here
if {[catch {set channel [open UGridField.vtk w]}] == 0 } {
   close $channel

vtkDataObjectWriter write
    write SetInput [ds2do GetOutput]
    write SetFileName "UGridField.vtk"
    write Write

# Read the field and convert to unstructured grid.
vtkDataObjectReader dor
    dor SetFileName "UGridField.vtk"
vtkDataObjectToDataSetFilter do2ds
    do2ds SetInput [dor GetOutput]
    do2ds SetDataSetTypeToUnstructuredGrid
    do2ds SetPointComponent 0 Points 0 
    do2ds SetPointComponent 1 Points 1 
    do2ds SetPointComponent 2 Points 2 
    do2ds SetCellTypeComponent CellTypes 0
    do2ds SetCellConnectivityComponent Cells 0
vtkFieldDataToAttributeDataFilter fd2ad
    fd2ad SetInput [do2ds GetUnstructuredGridOutput]
    fd2ad SetInputFieldToDataObjectField
    fd2ad SetOutputAttributeDataToPointData
    fd2ad SetVectorComponent 0 displacement9 0 
    fd2ad SetVectorComponent 1 displacement9 1 
    fd2ad SetVectorComponent 2 displacement9 2 
    fd2ad SetScalarComponent 0 thickness9 0 
    
# Now start visualizing
vtkWarpVector warp
    warp SetInput [fd2ad GetUnstructuredGridOutput]

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

vtkContourFilter cf
    cf SetInput [connect2 GetOutput]
    cf SetValue 0 .5
vtkPolyDataMapper contourMapper
    contourMapper SetInput [cf GetOutput]
vtkActor contours
    contours SetMapper contourMapper

# Create graphics stuff
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
ren1 AddActor moldActor
ren1 AddActor parisonActor
ren1 AddActor contours
[ren1 GetActiveCamera] Azimuth 60
[ren1 GetActiveCamera] Roll -90
[ren1 GetActiveCamera] Dolly 3
ren1 ResetCameraClippingRange
ren1 SetBackground 1 1 1
renWin SetSize 375 200

iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}
if {[info commands rtExMath] != ""} { 
    file delete -force UGridField.vtk
}
}

# prevent the tk window from showing up then start the event loop
wm withdraw .



