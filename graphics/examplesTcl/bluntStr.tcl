catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# Create dashed streamlines

source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read data
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/bluntfinxyz.bin"
    pl3d SetQFileName "$VTK_DATA/bluntfinq.bin"
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
vtkPolyDataMapper mapStreamers
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
vtkPolyDataMapper wallMap
    wallMap SetInput [wall GetOutput]
    wallMap ScalarVisibilityOff
vtkActor wallActor
    wallActor SetMapper wallMap
    eval [wallActor GetProperty] SetColor 0.8 0.8 0.8

# fin
# 
vtkStructuredGridGeometryFilter fin
    fin SetInput [pl3d GetOutput]
    fin SetExtent 0 100 0 100 0 0
vtkPolyDataMapper finMap
    finMap SetInput [fin GetOutput]
    finMap ScalarVisibilityOff
vtkActor finActor
    finActor SetMapper finMap
    eval [finActor GetProperty] SetColor 0.8 0.8 0.8

# outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    set outlineProp [outlineActor GetProperty]
    eval $outlineProp SetColor 1 1 1

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor streamersActor
ren1 AddActor wallActor
ren1 AddActor finActor
ren1 SetBackground 0 0 0
renWin SetSize 700 500

vtkCamera cam1
  cam1 SetFocalPoint 2.87956 4.24691 2.73135
  cam1 SetPosition -3.46307 16.7005 29.7406
  cam1 SetViewAngle 30
  cam1 SetViewUp 0.127555 0.911749 -0.390441
  cam1 SetClippingRange 1 50

ren1 SetActiveCamera cam1

iren Initialize
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

#renWin SetFileName bluntStr.tcl.ppm
#renWin SaveImageAsPPM
