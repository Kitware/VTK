catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

source $VTK_TCL/vtkInt.tcl
source $VTK_TCL/colors.tcl

# Create renderer stuff
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# create pipeline
#
vtkSphereSource sphere
    sphere SetRadius 1
vtkPolyDataMapper mapper
    mapper SetInput [sphere GetOutput]
vtkActor actor
    actor SetMapper mapper
    eval [actor GetProperty] SetColor $peacock

vtkOutlineCornerFilter outline
    outline SetInput [sphere GetOutput]
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    [outlineActor GetProperty] SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren1 AddActor outlineActor
ren1 AddActor actor
ren1 SetBackground 1 1 1
set camera [ren1 GetActiveCamera]
renWin SetSize 300 300
iren Initialize
$camera Azimuth 30
$camera Elevation 30

# prevent the tk window from showing up then start the event loop
wm withdraw .

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

renWin Render

vtkWindowToImageFilter windowToImage
    windowToImage SetInput renWin

vtkTIFFWriter writer
    writer SetFileName outlineCorner.tcl.tif
    writer SetInput [windowToImage GetOutput]
#writer Write



