#
# This example shows how to add an observer to a Tcl program. It extends
# the Step1/Tcl/Cone.tcl Tcl example (see that example for information on
# the basic setup). 
#
# VTK uses a command/observer design pattern. That is, observers watch for
# particular events that any vtkObject (or subclass) may invoke on
# itself. For example, the vtkRenderer invokes a "StartEvent" as it begins
# to render. Here we add an observer that invokes a command when this event
# is observed.
#

#
# First we include the VTK Tcl packages which will make available 
# all of the vtk commands to Tcl
#
package require vtk

#
# Here we define our callback
#
proc myCallback {} {
    puts "Starting to render"
}

#
# Next we create the pipelinne
#
vtkConeSource cone
cone SetHeight 3.0
cone SetRadius 1.0
cone SetResolution 10

vtkPolyDataMapper coneMapper
coneMapper SetInput [cone GetOutput]
vtkActor coneActor
coneActor SetMapper coneMapper

vtkRenderer ren1 
ren1 AddActor coneActor
ren1 SetBackground 0.1 0.2 0.4

# here we setup the callback
ren1 AddObserver StartEvent myCallback

vtkRenderWindow renWin
renWin AddRenderer ren1
renWin SetSize 300 300

#
# now we loop over 360 degreeees and render the cone each time
#
for {set i 0} {$i < 360} {incr i} {
   after 10
   # render the image
   renWin Render
   # rotate the active camera by one degree
   [ren1 GetActiveCamera] Azimuth 1
}

#
# Free up any objects we created
#
vtkCommand DeleteAllObjects

#
# exit the application
#
exit



