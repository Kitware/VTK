#
# This example demonstrates how to add observers to an applicaiton. It
# extends the Step1/Tcl/Cone.tcl example by adding an oberver. See Step1 for
# more information on the basics of the pipeline
#
#
# First we include the vtktcl package which will make available 
# all of the vtk commands to Tcl
#
package require vtktcl

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



