catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create the RenderWindow, Renderer and Interactor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "$VTK_DATA/combxyz.bin"
    pl3d SetQFileName "$VTK_DATA/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
vtkExtractGrid extract
    extract SetVOI 30 30 -1000 1000 -1000 1000
    extract SetSampleRate 1 2 3
    extract SetInput [pl3d GetOutput]
    extract IncludeBoundaryOn
vtkDataSetMapper cutterMapper
    cutterMapper SetInput [extract GetOutput]
    cutterMapper SetScalarRange .18 .7
vtkActor cut
    cut SetMapper cutterMapper

vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor cut

ren1 SetBackground 1 1 1
renWin SetSize 300 180

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 2.64586 47.905
$cam1 SetFocalPoint 8.931 0.358127 31.3526
$cam1 SetPosition 29.7111 -0.688615 37.1495
$cam1 SetViewUp -0.268328 0.00801595 0.963294

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render

renWin SetFileName "sampleGridToBoundary.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
