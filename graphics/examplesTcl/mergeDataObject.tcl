catch {load vtktcl}
# Demonstrate the separation of datasets (i.e., topology/geometry) from
# attribute data (i.e., a field) into different files.
source ../../examplesTcl/vtkInt.tcl

# create readers
vtkUnstructuredGridReader readGeom
    readGeom SetFileName "../../../vtkdata/blowGeom.vtk"
    readGeom DebugOn
vtkDataObjectReader readAttr
    readAttr SetFileName "../../../vtkdata/blowAttr.vtk"
    readAttr SetFieldDataName "time9"
    readAttr DebugOn

# combine data and build scalars and vectors
vtkMergeDataObjectFilter merge
    merge SetInput [readGeom GetOutput]
    merge SetDataObject [readAttr GetOutput]
vtkFieldDataToAttributeDataFilter fd2ad
    fd2ad SetInput [merge GetOutput]
    fd2ad SetOutputAttributeDataToPointData
    fd2ad SetVectorComponent 0 displacement 0 
    fd2ad SetVectorComponent 1 displacement 1 
    fd2ad SetVectorComponent 2 displacement 2 
    fd2ad SetScalarComponent 0 thickness 0 

# warp data with vectors
vtkWarpVector warp
    warp SetInput [fd2ad GetUnstructuredGridOutput]

# extract mold from mesh using connectivity
vtkConnectivityFilter connect
    connect SetInput [warp GetOutput]
    connect SetExtractionModeToSpecifiedRegions
    connect AddSpecifiedRegion 0
    connect AddSpecifiedRegion 1
vtkDataSetMapper moldMapper
    moldMapper SetInput [readGeom GetOutput]
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
[ren1 GetActiveCamera] Dolly 2
ren1 SetBackground 1 1 1
renWin SetSize 750 400

iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}

#renWin SetFileName "blow.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



