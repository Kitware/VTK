catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkPLOT3DReader pl3d2
    pl3d2 SetXYZFileName "$VTK_DATA/combxyz.bin"
    pl3d2 SetQFileName "$VTK_DATA/combq.bin"
    pl3d2 SetScalarFunctionNumber 153
    pl3d2 Update

vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update

vtkContourFilter iso
    iso SetInput [pl3d GetOutput]
    iso SetValue 0 .24

vtkProbeFilter probe2
  probe2 SetInput [iso GetOutput]
  probe2 SetSource [pl3d2 GetOutput]

vtkCastToConcrete cast2 
  cast2 SetInput [probe2 GetOutput]

vtkContourFilter isoLines
  isoLines SetInput [cast2 GetOutput]
  isoLines GenerateValues 10 0 1400

vtkStripper isoStrips
  isoStrips SetInput [isoLines GetOutput]

vtkTubeFilter isoTubes
  isoTubes SetInput [isoStrips GetOutput]
  isoTubes SetRadius .1
  isoTubes SetNumberOfSides 5

vtkPolyDataMapper isoLinesMapper
    isoLinesMapper SetInput [isoTubes GetOutput]
    isoLinesMapper ScalarVisibilityOn
    isoLinesMapper SetScalarRange 0 1400

vtkActor isoLinesActor
    isoLinesActor SetMapper isoLinesMapper
    eval [isoLinesActor GetProperty] SetColor $bisque

vtkPolyDataMapper isoMapper
    isoMapper SetInput [iso GetOutput]
    isoMapper ScalarVisibilityOff

vtkActor isoActor
    isoActor SetMapper isoMapper
    eval [isoActor GetProperty] SetColor $bisque

vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor isoLinesActor
ren1 AddActor isoActor
ren1 SetBackground 1 1 1
renWin SetSize 640 480
ren1 SetBackground 0.1 0.2 0.4

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 SetViewUp -0.16123 0.264271 0.950876
$cam1 Dolly 1.2

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render
#renWin SetFileName "isoIntersect.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .


