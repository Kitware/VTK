package require vtk
package require vtkinteraction
package require vtktesting

## Generate a rectilinear grid from a field.
##

# get the interactor ui

# Create a reader and write out the field
vtkDataSetReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/RectGrid2.vtk"
vtkDataSetToDataObjectFilter ds2do
    ds2do SetInput [reader GetOutput]
if {[catch {set channel [open RGridField.vtk w]}] == 0 } {
   close $channel
vtkDataObjectWriter writer
    writer SetInput [ds2do GetOutput]
    writer SetFileName "RGridField.vtk"
    writer Write

# Read the field
#
vtkDataObjectReader dor
    dor SetFileName "RGridField.vtk"
vtkDataObjectToDataSetFilter do2ds
    do2ds SetInput [dor GetOutput]
    do2ds SetDataSetTypeToRectilinearGrid
    do2ds SetDimensionsComponent Dimensions 0 
    do2ds SetPointComponent 0 XCoordinates 0
    do2ds SetPointComponent 1 YCoordinates 0 
    do2ds SetPointComponent 2 ZCoordinates 0
vtkFieldDataToAttributeDataFilter fd2ad
    fd2ad SetInput [do2ds GetRectilinearGridOutput]
    fd2ad SetInputFieldToDataObjectField
    fd2ad SetOutputAttributeDataToPointData
    fd2ad SetVectorComponent 0 vectors 0 
    fd2ad SetVectorComponent 1 vectors 1 
    fd2ad SetVectorComponent 2 vectors 2 
    fd2ad SetScalarComponent 0 scalars 0 
    fd2ad Update

# create pipeline
#
vtkRectilinearGridGeometryFilter plane
    plane SetInput [fd2ad GetRectilinearGridOutput]
    plane SetExtent 0 100 0 100 15 15 
vtkWarpVector warper
    warper SetInput [plane GetOutput]
    warper SetScaleFactor 0.05
vtkDataSetMapper planeMapper
    planeMapper SetInput [warper GetOutput]
    planeMapper SetScalarRange 0.197813 0.710419
vtkActor planeActor
    planeActor SetMapper planeMapper

vtkPlane cutPlane
    eval cutPlane SetOrigin [[fd2ad GetOutput] GetCenter]
    cutPlane SetNormal 1 0 0
vtkCutter planeCut
    planeCut SetInput [fd2ad GetRectilinearGridOutput]
    planeCut SetCutFunction cutPlane
vtkDataSetMapper cutMapper
    cutMapper SetInput [planeCut GetOutput]
    eval cutMapper SetScalarRange \
    [[[[fd2ad GetOutput] GetPointData] GetScalars] GetRange]
vtkActor cutActor
    cutActor SetMapper cutMapper

vtkContourFilter iso
    iso SetInput [fd2ad GetRectilinearGridOutput]
    iso SetValue 0 0.7
vtkPolyDataNormals normals
    normals SetInput [iso GetOutput]
    normals SetFeatureAngle 45
vtkPolyDataMapper isoMapper
    isoMapper SetInput [normals GetOutput]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $bisque
    eval [isoActor GetProperty] SetRepresentationToWireframe

vtkStreamLine streamer
    streamer SetInput [fd2ad GetOutput]
    streamer SetStartPosition -1.2 -0.1 1.3
    streamer SetMaximumPropagationTime 500
    streamer SetStepLength 0.05
    streamer SetIntegrationStepLength 0.05
    streamer SetIntegrationDirectionToIntegrateBothDirections

vtkTubeFilter streamTube
    streamTube SetInput [streamer GetOutput]
    streamTube SetRadius 0.025
    streamTube SetNumberOfSides 6
    streamTube SetVaryRadiusToVaryRadiusByVector
vtkPolyDataMapper mapStreamTube
    mapStreamTube SetInput [streamTube GetOutput]
    eval mapStreamTube SetScalarRange \
       [[[[fd2ad GetOutput] GetPointData] GetScalars] GetRange]
vtkActor streamTubeActor
    streamTubeActor SetMapper mapStreamTube
    [streamTubeActor GetProperty] BackfaceCullingOn

vtkOutlineFilter outline
    outline SetInput [fd2ad GetRectilinearGridOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor $black

# Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor planeActor
ren1 AddActor cutActor
ren1 AddActor isoActor
ren1 AddActor streamTubeActor

ren1 SetBackground 1 1 1
renWin SetSize 300 300

[ren1 GetActiveCamera] SetPosition 0.0390893 0.184813 -3.94026 
[ren1 GetActiveCamera] SetFocalPoint -0.00578326 0 0.701967 
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0.00850257 0.999169 0.0398605 
[ren1 GetActiveCamera] SetClippingRange 3.08127 6.62716 

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
if {[info commands rtExMath] != ""} { 
    file delete -force RGridField.vtk
}
}

# prevent the tk window from showing up then start the event loop
wm withdraw .



