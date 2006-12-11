package require vtk
package require vtkinteraction

vtkConeSource cone
cone SetHeight 3.0
cone SetRadius 1.0
cone SetResolution 10

vtkPolyDataMapper coneMapper
coneMapper SetInputConnection [cone GetOutputPort]

# Actor for opacity as a property value.
vtkActor coneActor
coneActor SetMapper coneMapper
[coneActor GetProperty] SetOpacity 0.5



# Actor for opacity thru LUT.
vtkElevationFilter elevation 
elevation SetInputConnection [cone GetOutputPort]

vtkPolyDataMapper coneMapper2
coneMapper2 SetInputConnection [elevation GetOutputPort]

vtkLookupTable lut
lut SetAlphaRange 0.9 0.1
lut SetHueRange 0 0
lut SetSaturationRange 1 1
lut SetValueRange 1 1

coneMapper2 SetLookupTable lut
coneMapper2 SetScalarModeToUsePointData
coneMapper2 SetScalarVisibility 1

vtkActor coneActorLUT
coneActorLUT SetMapper coneMapper2
coneActorLUT SetPosition 0 1.0 0
[coneActorLUT GetProperty] SetOpacity 0.99


# Actor for opacity thru texture.
vtkPNGReader reader
reader SetFileName "$VTK_DATA_ROOT/Data/alphachannel.png"
reader Update

vtkSphereSource sphere

vtkTexture texture
texture SetInput [reader GetOutput]

vtkPolyDataMapper coneMapper3
coneMapper3 SetInputConnection [sphere GetOutputPort]

vtkActor coneActorTexture
coneActorTexture SetTexture texture 
coneActorTexture SetMapper coneMapper3
coneActorTexture SetPosition 0 -1.0 0
[coneActorTexture GetProperty] SetColor 0.5 0.5 1
[coneActorTexture GetProperty] SetOpacity 0.99




#
# Create the Renderer and assign actors to it. A renderer is like a
# viewport. It is part or all of a window on the screen and it is responsible
# for drawing the actors it has.  We also set the background color here.
#
vtkRenderer ren1 
ren1 AddActor coneActor
ren1 AddActor coneActorLUT
ren1 AddActor coneActorTexture
ren1 SetBackground 0.1 0.2 0.4


#
# Finally we create the render window which will show up on the screen
# We put our renderer into the render window using AddRenderer. We also
# set the size to be 300 pixels by 300.
#
vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetSize 300 300

# 
# The vtkRenderWindowInteractor class watches for events (e.g., keypress,
# mouse) in the vtkRenderWindow. These events are translated into
# event invocations that VTK understands (see VTK/Common/vtkCommand.h
# for all events that VTK processes). Then observers of these VTK
# events can process them as appropriate.
vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

#
# By default the vtkRenderWindowInteractor instantiates an instance
# of vtkInteractorStyle. vtkInteractorStyle translates a set of events
# it observes into operations on the camera, actors, and/or properties
# in the vtkRenderWindow associated with the vtkRenderWinodwInteractor. 
# Here we specify a particular interactor style.
vtkInteractorStyleTrackballCamera style
iren SetInteractorStyle style

#
# Unlike the previous scripts where we performed some operations and then
# exited, here we leave an event loop running. The user can use the mouse
# and keyboard to perform the operations on the scene according to the
# current interaction style.
#

#
# Another feature of Tcl/Tk is that in VTK a simple GUI for typing in
# interpreted Tcl commands is provided. The so-called vtkInteractor appears
# when the user types the "u" (for user) keypress. The "u" keypress is
# translated into a UserEvent by the vtkRenderWindowInteractor. We observe
# this event and invoke a commands to deiconify the vtkInteractor. (The
# vtkInteractor is defined in the vtkinteraction package reference at the
# beginning of this script.)
# 
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

#
# Initialize the event loop. The actual interaction starts after 
# wm withdraw . with the Tk event loop. Once the render window appears, 
# mouse in the window to move the camera. Note that keypress-e exits this
# example. (Look in vtkInteractorStyle.h for a summary of events, or
# the appropriate Doxygen documentation.)
#
iren Initialize

#
# Since we are in the Tcl/Tk environment, we prevent the empty "."
# window from appearing with the Tk "withdraw" command.
#
wm withdraw .




set camera [ren1 GetActiveCamera]
$camera SetPosition 9 -1 3
$camera SetViewAngle 30
$camera SetViewUp 0.05 0.96 0.24
$camera SetFocalPoint 0 0.25 0
