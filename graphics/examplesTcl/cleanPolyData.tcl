catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# this is a tcl version of old franFace
# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create a cyberware source
#
vtkPolyDataReader cyber
    cyber SetFileName "$VTK_DATA/fran_cut.vtk"

vtkCleanPolyData clean
  clean SetInput [cyber GetOutput]
  clean ToleranceIsAbsoluteOn
  clean SetAbsoluteTolerance .005

vtkPolyDataMapper cyberMapper
    cyberMapper SetInput [clean GetOutput]

vtkPNMReader pnm1
    pnm1 SetFileName "$VTK_DATA/fran_cut.ppm"

vtkTexture atext
  atext SetInput [pnm1 GetOutput]
  atext InterpolateOn

vtkActor cyberActor
  cyberActor SetMapper cyberMapper
  cyberActor SetTexture atext

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cyberActor
ren1 SetBackground 1 1 1
renWin SetSize 321 321

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
[ren1 GetActiveCamera] SetPosition 0.407534 -0.1268 -0.0565854 
[ren1 GetActiveCamera] SetFocalPoint 0.0524175 -0.1268 -0.0565854 
[ren1 GetActiveCamera] SetViewAngle 30
[ren1 GetActiveCamera] SetViewUp 0 1 0 
[ren1 GetActiveCamera] SetClippingRange 0.247254 0.491749 

iren Initialize

renWin SetFileName "cleanPolyData.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


