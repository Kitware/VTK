catch {load vtktcl}
catch {load vtktcl}
# Create dashed streamlines

source vtkInt.tcl
source colors.tcl

# Create the render master
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]

# read data
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "../../../data/bluntfinxyz.bin"
    pl3d SetQFileName "../../../data/bluntfinq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

#streamers
#
vtkLineSource seeds
    seeds SetResolution 25
    seeds SetPoint1 -6.5 0.25 0.10
    seeds SetPoint2 -6.5 0.25 5.0
vtkDashedStreamLine streamers
    streamers SetInput [pl3d GetOutput]
    streamers SetSource [seeds GetOutput]
    streamers SetMaximumPropagationTime 25
    streamers SetStepLength 0.25
    streamers Update
vtkPolyMapper mapStreamers
    mapStreamers SetInput [streamers GetOutput]
    eval mapStreamers SetScalarRange \
       [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange]
vtkActor streamersActor
    streamersActor SetMapper mapStreamers

# wall
#
vtkStructuredGridGeometryFilter wall
    wall SetInput [pl3d GetOutput]
    wall SetExtent 0 100 0 0 0 100
vtkPolyMapper wallMap
    wallMap SetInput [wall GetOutput]
    wallMap ScalarsVisibleOff
vtkActor wallActor
    wallActor SetMapper wallMap
    eval [wallActor GetProperty] SetColor 0.8 0.8 0.8

# fin
# 
vtkStructuredGridGeometryFilter fin
    fin SetInput [pl3d GetOutput]
    fin SetExtent 0 100 0 100 0 0
vtkPolyMapper finMap
    finMap SetInput [fin GetOutput]
    finMap ScalarsVisibleOff
vtkActor finActor
    finActor SetMapper finMap
    eval [finActor GetProperty] SetColor 0.8 0.8 0.8

# outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    set outlineProp [outlineActor GetProperty]
    eval $outlineProp SetColor 1 1 1

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor
$ren1 AddActors streamersActor
$ren1 AddActors wallActor
$ren1 AddActors finActor
$ren1 SetBackground 0 0 0
$renWin SetSize 700 500

vtkCamera cam1
  cam1 SetFocalPoint 2.87956 4.24691 2.73135
  cam1 SetPosition -3.46307 16.7005 29.7406
  cam1 CalcViewPlaneNormal
  cam1 SetViewAngle 30
  cam1 SetViewUp 0.127555 0.911749 -0.390441
$ren1 SetActiveCamera cam1

$iren Initialize
$renWin Render

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

$renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

#$renWin SetFileName bluntStr.tcl.ppm
#$renWin SaveImageAsPPM
