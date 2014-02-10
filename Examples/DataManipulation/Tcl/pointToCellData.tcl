# This example demonstrates the conversion of point data to cell data.
# The conversion is necessary because we want to threshold data based
# on cell scalar values.

package require vtk
package require vtkinteraction

# Read some data with point data attributes. The data is from a plastic
# blow molding process (e.g., to make plastic bottles) and consists of two
# logical components: a mold and a parison. The parison is the
# hot plastic that is being molded, and the mold is clamped around the
# parison to form its shape.
vtkUnstructuredGridReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/blow.vtk"
    reader SetScalarsName "thickness9"
    reader SetVectorsName "displacement9"

# Convert the point data to cell data. The point data is passed through the
# filter so it can be warped. The vtkThresholdFilter then thresholds based
# on cell scalar values and extracts a portion of the parison whose cell
# scalar values lie between 0.25 and 0.75.
vtkPointDataToCellData p2c
    p2c SetInputConnection [reader GetOutputPort]
    p2c PassPointDataOn
vtkWarpVector warp
    warp SetInputConnection [p2c GetOutputPort]
vtkThreshold thresh
    thresh SetInputConnection [warp GetOutputPort]
    thresh ThresholdBetween 0.25 0.75
    thresh SetInputArrayToProcess 1 0 0 0 "thickness9"
#    thresh SetAttributeModeToUseCellData

# This is used to extract the mold from the parison.
vtkConnectivityFilter connect
    connect SetInputConnection [thresh GetOutputPort]
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

# The threshold filter has been used to extract the parison.
vtkConnectivityFilter connect2
    connect2 SetInputConnection [thresh GetOutputPort]
vtkGeometryFilter parison
    parison SetInputConnection [connect2 GetOutputPort]
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

# We generate some contour lines on the parison.
vtkContourFilter cf
    cf SetInputConnection [connect2 GetOutputPort]
    cf SetValue 0 .5
vtkPolyDataMapper contourMapper
    contourMapper SetInputConnection [cf GetOutputPort]
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

ren1 ResetCamera
[ren1 GetActiveCamera] Azimuth 60
[ren1 GetActiveCamera] Roll -90
[ren1 GetActiveCamera] Dolly 2
ren1 ResetCameraClippingRange

ren1 SetBackground 1 1 1
renWin SetSize 750 400

iren Initialize
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
iren Start