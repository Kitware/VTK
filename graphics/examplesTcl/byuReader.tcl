catch {load vtktcl}
# get the interactor ui
source ../../examplesTcl/vtkInt.tcl
source ../../examplesTcl/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# read data
#
vtkBYUReader byu
  byu SetGeometryFileName brain.g
  byu SetScalarFileName brain.s
  byu SetDisplacementFileName brain.d
  byu Update

vtkPolyDataMapper mapper
  mapper SetInput [byu GetOutput]
  eval mapper SetScalarRange [[byu GetOutput] GetScalarRange]

vtkActor brain
  brain SetMapper mapper


# Add the actors to the renderer, set the background and size
#
ren1 AddActor brain

renWin SetSize 320 240

set cam1 [ren1 GetActiveCamera]
$cam1 SetPosition 152.589  -135.901 173.068
$cam1 SetFocalPoint 146.003 22.3839 0.260541
$cam1 SetViewUp -0.255578 -0.717754 -0.647695

iren Initialize
renWin Render

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .

#renWin SetFileName byuReader.tcl.ppm
#renWin SaveImageAsPPM


