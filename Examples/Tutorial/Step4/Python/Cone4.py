#!/usr/bin/env python
#
# This example demonstrates the creation of multiple actors and the
# manipulation of their properties and transformations. It is a
# derivative of Cone.py, see that example for more information.
#

import vtk
import time

# 
# Next we create an instance of vtkConeSource and set some of its
# properties. The instance of vtkConeSource "cone" is part of a visualization
# pipeline (it is a source process object); it produces data (output type is
# vtkPolyData) which other filters may process.
#
cone = vtk.vtkConeSource ()
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
coneMapper = vtk.vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())

# 
# Create an actor to represent the first cone. The actor's properties are
# modified to give it different surface properties. By default, an actor
# is create with a property so the GetProperty() method can be used.
#
coneActor = vtk.vtkActor()
coneActor.SetMapper(coneMapper)
coneActor.GetProperty().SetColor(0.2, 0.63, 0.79)
coneActor.GetProperty().SetDiffuse(0.7)
coneActor.GetProperty().SetSpecular(0.4)
coneActor.GetProperty().SetSpecularPower(20)

#
# Create a property and directly manipulate it. Assign it to the
# second actor.
#
property = vtk.vtkProperty()
property.SetColor(1.0, 0.3882, 0.2784)
property.SetDiffuse(0.7)
property.SetSpecular(0.4)
property.SetSpecularPower(20)

#
# Create a second actor and a property. The property is directly
# manipulated and then assigned to the actor. In this way, a single
# property can be shared among many actors. Note also that we use the
# same mapper as the first actor did. This way we avoid duplicating
# geometry, which may save lots of memory if the geoemtry is large.
coneActor2 = vtk.vtkActor()
coneActor2.SetMapper(coneMapper)
coneActor2.GetProperty().SetColor(0.2, 0.63, 0.79)
coneActor2.SetProperty(property)
coneActor2.SetPosition(0, 2, 0)

#
# Create the Renderer and assign actors to it. A renderer is like a
# viewport. It is part or all of a window on the screen and it is responsible
# for drawing the actors it has.  We also set the background color here.
#
ren1 = vtk.vtkRenderer()
ren1.AddActor(coneActor)
ren1.AddActor(coneActor2)
ren1.SetBackground(0.1, 0.2, 0.4)

#
# Finally we create the render window which will show up on the screen
# We put our renderer into the render window using AddRenderer. We also
# set the size to be 300 pixels by 300.
#
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(300, 300)

#
# Now we loop over 360 degreeees and render the cone each time.
#
for i in range(0,360):
    time.sleep(0.03)
    
    renWin.Render()
    ren1.GetActiveCamera().Azimuth( 1 )

