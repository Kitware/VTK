package require vtk
package require vtkinteraction
package require vtktesting

set VTK_VARY_RADIUS_BY_VECTOR 2

# create pipeline
#
vtkDataSetReader reader
    reader SetFileName "$VTK_DATA_ROOT/Data/RectGrid2.vtk"
    reader Update
vtkCastToConcrete toRectilinearGrid
    toRectilinearGrid SetInput [reader GetOutput] 
vtkRectilinearGridGeometryFilter plane
    plane SetInput [toRectilinearGrid GetRectilinearGridOutput]
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
    eval cutPlane SetOrigin [[reader GetOutput] GetCenter]
    cutPlane SetNormal 1 0 0
vtkCutter planeCut
    planeCut SetInput [toRectilinearGrid GetRectilinearGridOutput]
    planeCut SetCutFunction cutPlane
vtkDataSetMapper cutMapper
    cutMapper SetInput [planeCut GetOutput]
    eval cutMapper SetScalarRange \
      [[[[reader GetOutput] GetPointData] GetScalars] GetRange]
vtkActor cutActor
    cutActor SetMapper cutMapper

vtkContourFilter iso
    iso SetInput [toRectilinearGrid GetRectilinearGridOutput]
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
    streamer SetInput [reader GetOutput]
    streamer SetStartPosition -1.2 -0.1 1.3
    streamer SetMaximumPropagationTime 500
    streamer SetStepLength 0.05
    streamer SetIntegrationStepLength 0.05
    streamer SetIntegrationDirectionToIntegrateBothDirections

vtkTubeFilter streamTube
    streamTube SetInput [streamer GetOutput]
    streamTube SetRadius 0.025
    streamTube SetNumberOfSides 6
    streamTube SetVaryRadius $VTK_VARY_RADIUS_BY_VECTOR
vtkPolyDataMapper mapStreamTube
    mapStreamTube SetInput [streamTube GetOutput]
    eval mapStreamTube SetScalarRange \
       [[[[reader GetOutput] GetPointData] GetScalars] GetRange]
vtkActor streamTubeActor
    streamTubeActor SetMapper mapStreamTube
    [streamTubeActor GetProperty] BackfaceCullingOn

vtkOutlineFilter outline
    outline SetInput [toRectilinearGrid GetRectilinearGridOutput]
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



