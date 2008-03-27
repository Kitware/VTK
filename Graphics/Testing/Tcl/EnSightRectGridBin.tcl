package require vtk
package require vtkinteraction
package require vtktesting

set VTK_VARY_RADIUS_BY_VECTOR 2

# create pipeline
#
vtkGenericEnSightReader reader
# Make sure all algorithms use the composite data pipeline
vtkCompositeDataPipeline cdp
reader SetDefaultExecutivePrototype cdp
    reader SetCaseFileName "$VTK_DATA_ROOT/Data/EnSight/RectGrid_bin.case"
    reader Update
vtkCastToConcrete toRectilinearGrid
#    toRectilinearGrid SetInputConnection [reader GetOutputPort] 
toRectilinearGrid SetInput [[reader GetOutput] GetBlock 0]
vtkRectilinearGridGeometryFilter plane
    plane SetInput [toRectilinearGrid GetRectilinearGridOutput]
    plane SetExtent 0 100 0 100 15 15 
vtkTriangleFilter tri
    tri SetInputConnection [plane GetOutputPort]
vtkWarpVector warper
    warper SetInputConnection [tri GetOutputPort]
    warper SetScaleFactor 0.05
vtkDataSetMapper planeMapper
    planeMapper SetInputConnection [warper GetOutputPort]
    planeMapper SetScalarRange 0.197813 0.710419
vtkActor planeActor
    planeActor SetMapper planeMapper

vtkPlane cutPlane
#    eval cutPlane SetOrigin [[reader GetOutput] GetCenter]
eval cutPlane SetOrigin [[[reader GetOutput] GetBlock 0] GetCenter]
    cutPlane SetNormal 1 0 0
vtkCutter planeCut
    planeCut SetInput [toRectilinearGrid GetRectilinearGridOutput]
    planeCut SetCutFunction cutPlane
vtkDataSetMapper cutMapper
    cutMapper SetInputConnection [planeCut GetOutputPort]
    eval cutMapper SetScalarRange \
  [[[[[reader GetOutput] GetBlock 0] GetPointData] GetScalars] GetRange]
vtkActor cutActor
    cutActor SetMapper cutMapper

vtkContourFilter iso
    iso SetInput [toRectilinearGrid GetRectilinearGridOutput]
    iso SetValue 0 0.7
vtkPolyDataNormals normals
    normals SetInputConnection [iso GetOutputPort]
    normals SetFeatureAngle 45
vtkPolyDataMapper isoMapper
    isoMapper SetInputConnection [normals GetOutputPort]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $bisque
    eval [isoActor GetProperty] SetRepresentationToWireframe

vtkStreamLine streamer
#    streamer SetInputConnection [reader GetOutputPort]
streamer SetInput [[reader GetOutput] GetBlock 0]
    streamer SetStartPosition -1.2 -0.1 1.3
    streamer SetMaximumPropagationTime 500
    streamer SetStepLength 0.05
    streamer SetIntegrationStepLength 0.05
    streamer SetIntegrationDirectionToIntegrateBothDirections

vtkTubeFilter streamTube
    streamTube SetInputConnection [streamer GetOutputPort]
    streamTube SetRadius 0.025
    streamTube SetNumberOfSides 6
    streamTube SetVaryRadius $VTK_VARY_RADIUS_BY_VECTOR
vtkPolyDataMapper mapStreamTube
    mapStreamTube SetInputConnection [streamTube GetOutputPort]
    eval mapStreamTube SetScalarRange \
  [[[[[reader GetOutput] GetBlock 0] GetPointData] GetScalars] GetRange]
#       [[[[reader GetOutput] GetPointData] GetScalars] GetRange]
vtkActor streamTubeActor
    streamTubeActor SetMapper mapStreamTube
    [streamTubeActor GetProperty] BackfaceCullingOn

vtkOutlineFilter outline
    outline SetInput [toRectilinearGrid GetRectilinearGridOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInputConnection [outline GetOutputPort]
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
renWin SetSize 400 400

set cam1 [ren1 GetActiveCamera]
    $cam1 SetClippingRange 3.76213 10.712
    $cam1 SetFocalPoint -0.0842503 -0.136905 0.610234
    $cam1 SetPosition 2.53813 2.2678 -5.22172
    $cam1 SetViewUp -0.241047 0.930635 0.275343

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

reader SetDefaultExecutivePrototype {}
