catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl
source colors.tcl

# First create the render master
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]
vtkLight lgt

# create pipeline
#
vtkVolume16Reader v16
    v16 SetDataDimensions 128 128 
    [v16 GetOutput] SetOrigin 0.0 0.0 0.0
    v16 SetFileTypeLittleEndian
    v16 SetFilePrefix "../../../data/headsq/half"
    v16 SetImageRange 1 93
    v16 SetDataSpacing 1.6 1.6 1.5
vtkMarchingCubes iso
    iso SetInput [v16 GetOutput]
    iso SetValue 0 1150
vtkPolyMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper ScalarVisibilityOff
vtkActor isoActor
    isoActor SetMapper isoMapper
set isoProp [isoActor GetProperty]
eval $isoProp SetColor $antique_white

vtkOutlineFilter outline
    outline SetInput [v16 GetOutput]
vtkPolyMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
set outlineProp [outlineActor GetProperty]
#eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
$ren1 AddActors outlineActor
$ren1 AddActors isoActor
$ren1 SetBackground 1 1 1
$ren1 AddLights lgt
$renWin SetSize 500 500
$ren1 SetBackground 0.1 0.2 0.4
$renWin DoubleBufferOff

set cam1 [$ren1 GetActiveCamera]
$cam1 Elevation 90
$cam1 SetViewUp 0 0 -1
$cam1 Zoom 1.3
$cam1 Azimuth 180
eval lgt SetPosition [$cam1 GetPosition]
eval lgt SetFocalPoint [$cam1 GetFocalPoint]

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

$renWin Render
#$renWin SetFileName "headBone.tcl.ppm"
#$renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


