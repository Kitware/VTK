#!/usr/bin/env python
#
# This example demonstrates how to use multiple renderers within a
# render window. It is a variation of the Cone.py example. Please
# refer to that example for additional documentation.
#

from vtkpython import *
import time

# 
# Next we create an instance of vtkConeSource and set some of its
# properties. The instance of vtkConeSource "cone" is part of a visualization
# pipeline (it is a source process object); it produces data (output type is
# vtkPolyData) which other filters may process.
#
cone = vtkConeSource()
cone.SetHeight( 3.0 )
cone.SetRadius( 1.0 )
cone.SetResolution( 10 )

# 
# In this example we terminate the pipeline with a mapper process object.
# (Intermediate filters such as vtkShrinkPolyData could be inserted in
# between the source and the mapper.)  We create an instance of
# vtkPolyDataMapper to map the polygonal data into graphics primitives. We
# connect the output of the cone souece to the input of this mapper.
#
coneMapper = vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())

# 
# Create an actor to represent the cone. The actor orchestrates rendering of
# the mapper's graphics primitives. An actor also refers to properties via a
# vtkProperty instance, and includes an internal transformation matrix. We
# set this actor's mapper to be coneMapper which we created above.
#
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)

# 
# Create two renderers and assign actors to them. A renderer renders into a
# viewport within the vtkRenderWindow. It is part or all of a window on the
# screen and it is responsible for drawing the actors it has.  We also set
# the background color here. In this example we are adding the same actor
# to two different renderers; it is okay to add different actors to
# different renderers as well.
#
ren1 = vtkRenderer()
ren1.AddActor(coneActor)
ren1.SetBackground(0.1, 0.2, 0.4)
ren1.SetViewport(0.0, 0.0, 0.5, 1.0)

ren2 = vtkRenderer()
ren2.AddActor(coneActor)
ren2.SetBackground(0.1, 0.2, 0.4)
ren2.SetViewport(0.5, 0.0, 1.0, 1.0)

#
# Finally we create the render window which will show up on the screen.
# We add our two renderers into the render window using AddRenderer. We also
# set the size to be 600 pixels by 300.
#
renWin = vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.AddRenderer( ren2 )
renWin.SetSize(600, 300)

#
# Make one camera view 90 degrees from other.
#
ren1.GetActiveCamera().Azimuth(90)

#
# Now we loop over 360 degreeees and render the cone each time.
#
for i in range(0,360):
    time.sleep(0.03)
    
    renWin.Render()
    ren1.GetActiveCamera().Azimuth( 1 )
    ren2.GetActiveCamera().Azimuth( 1 )

  
#
# Free up any objects we created
#
cone = None
coneMapper = None
coneActor = None 
ren1 = None
ren2 = None
renWin = None
