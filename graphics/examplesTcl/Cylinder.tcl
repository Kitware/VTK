catch {load vtktcl}
# this is a tcl version of the Mace example
# include get the vtk interactor ui
source vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkCylinderSource cone
#vtkDiskSource cone
#cone SetInnerRadius 1.0
#cone SetOuterRadius 1.0
vtkPolyMapper coneMapper
    coneMapper SetInput [cone GetOutput]
vtkActor coneActor
    coneActor SetMapper coneMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor coneActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 450 450

# Get handles to some useful objects
#
iren SetUserMethod {wm deiconify .vtkInteract}
iren Initialize
renWin Render

#renWin SetFileName Cylinder.tcl.ppm
#renWin SaveImageAsPPM

set coneProp [coneActor GetProperty]

# prevent the tk window from showing up then start the event loop
wm withdraw .



