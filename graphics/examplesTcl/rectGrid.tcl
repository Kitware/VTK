## Display a rectilinear grid and some common visualization techniques
##
catch {load vtktcl}

# get the interactor ui
source vtkInt.tcl
source colors.tcl
source vtkInclude.tcl

# create pipeline
#
vtkRectilinearGridReader reader
    reader SetFilename "../../../data/RectGrid.vtk"
    reader DebugOn
    reader Update
vtkRectilinearGridGeometryFilter plane
    plane SetInput [reader GetOutput]
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
    planeCut SetInput [reader GetOutput]
    planeCut SetCutFunction cutPlane
vtkDataSetMapper cutMapper
    cutMapper SetInput [planeCut GetOutput]
    eval cutMapper SetScalarRange \
      [[[[reader GetOutput] GetPointData] GetScalars] GetRange]
vtkActor cutActor
    cutActor SetMapper cutMapper

vtkContourFilter iso
    iso SetInput [reader GetOutput]
    iso SetValue 0 0.7
vtkPolyNormals normals
    normals SetInput [iso GetOutput]
    normals SetFeatureAngle 45
vtkPolyMapper isoMapper
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
    streamer Update
vtkTubeFilter streamTube
    streamTube SetInput [streamer GetOutput]
    streamTube SetRadius 0.025
    streamTube SetNumberOfSides 6
    streamTube SetVaryRadius $VTK_VARY_RADIUS_BY_VECTOR
vtkPolyMapper mapStreamTube
    mapStreamTube SetInput [streamTube GetOutput]
    eval mapStreamTube SetScalarRange \
       [[[[reader GetOutput] GetPointData] GetScalars] GetRange]
vtkActor streamTubeActor
    streamTubeActor SetMapper mapStreamTube
    [streamTubeActor GetProperty] BackfaceCullingOn

vtkOutlineFilter outline
    outline SetInput [reader GetOutput]
vtkPolyMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor $black

# Graphics stuff
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor
$ren1 AddActors planeActor
$ren1 AddActors cutActor
$ren1 AddActors isoActor
$ren1 AddActors streamTubeActor

$ren1 SetBackground 1 1 1
$renWin SetSize 400 400

set cam1 [$ren1 GetActiveCamera]
    $cam1 SetClippingRange 1.04427 52.2137
    $cam1 SetFocalPoint 0.106213 0.0196539 2.10569
    $cam1 SetPosition -7.34153 4.54201 7.86157
    $cam1 ComputeViewPlaneNormal
    $cam1 SetViewUp 0.113046 0.847094 -0.519281

$iren Initialize

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .



