#!/usr/bin/env python
#
# This example creates a polygonal model of a cone, and then rendered it to
# the screen. It willrotate the cone 360 degrees and then exit. The basic
# setup of source . mapper . actor . renderer . renderwindow is 
# typical of most VTK programs.
#

from vtkpython import *

#
# Next we create an instance of vtkConeSource and set some of its 
# properties
#
cone = vtkConeSource()
cone.SetHeight( 3.0 )
cone.SetRadius( 1.0 )
cone.SetResolution( 10 )
  
#
# We create an instance of vtkPolyDataMapper to map the polygonal data 
# into graphics primitives. We connect the output of the cone souece 
# to the input of this mapper 
#
coneMapper = vtkPolyDataMapper()
coneMapper.SetInput( cone.GetOutput() )

#
# create an actor to represent the cone. The actor coordinates rendering of
# the graphics primitives for a mapper. We set this actor's mapper to be
# coneMapper which we created above.
#
coneActor = vtkActor()
coneActor.SetMapper( coneMapper )

#
# Create the Renderer and assign actors to it. A renderer is like a
# viewport. It is part or all of a window on the screen and it is
# responsible for drawing the actors it has.  We also set the background
# color here
#
ren1= vtkRenderer()
ren1.AddActor( coneActor )
ren1.SetBackground( 0.1, 0.2, 0.4 )

#
# Finally we create the render window which will show up on the screen
# We put our renderer into the render window using AddRenderer. We also
# set the size to be 300 pixels by 300
#
renWin = vtkRenderWindow()
renWin.AddRenderer( ren1 )
renWin.SetSize( 300, 300 )

#
# now we loop over 360 degreeees and render the cone each time
#
for i in range(0,360):
    renWin.Render()
    ren1.GetActiveCamera().Azimuth( 1 )
  
#
# Free up any objects we created
#
cone = None
coneMapper = None
coneActor = None 
ren1 = None
renWin = None

