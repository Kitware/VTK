#
# This example creates a polygonal model of a cone, and then renders it to
# the screen. It will rotate the cone 360 degrees and then exit. The basic
# setup of source -> mapper -> actor -> renderer -> renderwindow is 
# typical of most VTK programs.
#

#
# First we include the VTK Tcl packages which will make available 
# all of the VTK commands to Tcl.
#
package require vtk

# 
# Next we create an instance of vtkConeSource and set some of its
# properties. The instance of vtkConeSource "cone" is part of a visualization
# pipeline (it is a source process object); it produces data (output type is
# vtkPolyData) which other filters may process.
#
vtkConeSource cone
cone SetHeight 3.0
cone SetRadius 1.0
cone SetResolution 10

# 
# In this example we terminate the pipeline with a mapper process object.
# (Intermediate filters such as vtkShrinkPolyData could be inserted in
# between the source and the mapper.)  We create an instance of
# vtkPolyDataMapper to map the polygonal data into graphics primitives. We
# connect the output of the cone souece to the input of this mapper.
#
vtkPolyDataMapper coneMapper
coneMapper SetInput [cone GetOutput]

# 
# Create an actor to represent the cone. The actor orchestrates rendering of
# the mapper's graphics primitives. An actor also refers to properties via a
# vtkProperty instance, and includes an internal transformation matrix. We
# set this actor's mapper to be coneMapper which we created above.
#
vtkActor coneActor
coneActor SetMapper coneMapper

#
# Create the Renderer and assign actors to it. A renderer is like a
# viewport. It is part or all of a window on the screen and it is responsible
# for drawing the actors it has.  We also set the background color here.
#
vtkRenderer ren1 
ren1 AddActor coneActor
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
# Now we loop over 360 degreeees and render the cone each time.
#
for {set i 0} {$i < 360} {incr i} {
   after 10
   # render the image
   renWin Render
   # rotate the active camera by one degree
   [ren1 GetActiveCamera] Azimuth 1
}

#
# Free up any objects we created.
#
vtkCommand DeleteAllObjects

#
# Exit the application.
#
exit



