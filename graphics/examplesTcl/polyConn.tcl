catch {load vtktcl}
# get the interactor ui
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
    pl3d SetXYZFileName "../../data/bluntfinxyz.bin"
    pl3d SetQFileName "../../data/bluntfinq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

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

# planes to connect
vtkStructuredGridGeometryFilter plane1
    plane1 SetInput [pl3d GetOutput]
    plane1 SetExtent 10 10 0 100 0 100
vtkPolyConnectivityFilter conn
    conn SetInput [plane1 GetOutput]
    conn ScalarConnectivityOn
    conn SetScalarRange 1.5 4.0
vtkPolyMapper plane1Map
    plane1Map SetInput [conn GetOutput]
    eval plane1Map SetScalarRange [[pl3d GetOutput] GetScalarRange]
vtkActor plane1Actor
    plane1Actor SetMapper plane1Map
    [plane1Actor GetProperty] SetOpacity 0.999

# outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    set outlineProp [outlineActor GetProperty]
    eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor
$ren1 AddActors wallActor
$ren1 AddActors finActor
$ren1 AddActors plane1Actor
$ren1 SetBackground 1 1 1
$renWin SetSize 400 400

vtkCamera cam1
  cam1 SetClippingRange 1.51176 75.5879
  cam1 SetFocalPoint 2.33749 2.96739 3.61023
  cam1 SetPosition 10.8787 5.27346 15.8687
  cam1 SetViewAngle 30
  cam1 SetViewPlaneNormal 0.564986 0.152542 0.810877
  cam1 SetViewUp -0.0610856 0.987798 -0.143262
$ren1 SetActiveCamera cam1

$iren Initialize

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

$renWin SetFileName "polyConn.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .



