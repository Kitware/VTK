# This example demonstrates the use of vtkVectorText and vtkFollower.
# vtkVectorText is used to create 3D annotation.  vtkFollower is used to
# position the 3D text and to ensure that the text always faces the
# renderer's active camera (i.e., the text is always readable).

#
# First we include the VTK Tcl packages which will make available
# all of the vtk commands to Tcl
#
package require vtk
package require vtkinteraction

# Create the axes and the associated mapper and actor.
vtkAxes axes
    axes SetOrigin 0 0 0
vtkPolyDataMapper axesMapper
    axesMapper SetInput [axes GetOutput]
vtkActor axesActor
    axesActor SetMapper axesMapper

# Create the 3D text and the associated mapper and follower (a type of
# actor).  Position the text so it is displayed over the origin of the axes.
vtkVectorText atext
    atext SetText "Origin"
vtkPolyDataMapper textMapper
    textMapper SetInput [atext GetOutput]
vtkFollower textActor
    textActor SetMapper textMapper
    textActor SetScale 0.2 0.2 0.2
    textActor AddPosition 0 -0.1 0

# Create the Renderer, RenderWindow, and RenderWindowInteractor.
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer.
ren1 AddActor axesActor
ren1 AddActor textActor

# Zoom in closer.
[ren1 GetActiveCamera] Zoom 1.6

# Reset the clipping range of the camera; set the camera of the follower;
# render.
ren1 ResetCameraClippingRange
textActor SetCamera [ren1 GetActiveCamera]
renWin Render

# Set the user method (bound to key 'u')
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# Withdraw the default tk window.
wm withdraw .
