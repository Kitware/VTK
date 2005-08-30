# This example demonstrates the use of vtkSTLReader to load data into VTK from
# a file.  This example also uses vtkLODActor which changes its graphical
# representation of the data to maintain interactive performance.

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

# Create the reader and read a data file.  Connect the mapper and actor.
vtkSTLReader sr
    sr SetFileName $VTK_DATA_ROOT/Data/42400-IDGH.stl

vtkPolyDataMapper stlMapper
    stlMapper SetInputConnection [sr GetOutputPort]

vtkLODActor stlActor
    stlActor SetMapper stlMapper

# Create the Renderer, RenderWindow, and RenderWindowInteractor
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the render; set the background and size
#
ren1 AddActor stlActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# Zoom in closer
ren1 ResetCamera
set cam1 [ren1 GetActiveCamera]
$cam1 Zoom 1.4

# Set the user method (bound to key 'u')
iren AddObserver UserEvent {wm deiconify .vtkInteract}
iren Initialize

# Withdraw the default tk window
wm withdraw .
