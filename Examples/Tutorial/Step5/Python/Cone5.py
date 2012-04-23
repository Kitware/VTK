#!/usr/bin/env python

#
# This example introduces the concepts of user interaction with VTK.
# First, a different interaction style (than the default) is defined.
# Second, the interaction is started.
#
#

import vtk

#
# Next we create an instance of vtkConeSource and set some of its
# properties. The instance of vtkConeSource "cone" is part of a visualization
# pipeline (it is a source process object); it produces data (output type is
# vtkPolyData) which other filters may process.
#
cone = vtk.vtkConeSource()
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
coneMapper.SetInputConnection(cone.GetOutputPort())

#
# Create an actor to represent the cone. The actor orchestrates rendering of
# the mapper's graphics primitives. An actor also refers to properties via a
# vtkProperty instance, and includes an internal transformation matrix. We
# set this actor's mapper to be coneMapper which we created above.
#
coneActor = vtk.vtkActor()
coneActor.SetMapper(coneMapper)

#
# Create the Renderer and assign actors to it. A renderer is like a
# viewport. It is part or all of a window on the screen and it is responsible
# for drawing the actors it has.  We also set the background color here.
#
ren1 = vtk.vtkRenderer()
ren1.AddActor(coneActor)
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
# The vtkRenderWindowInteractor class watches for events (e.g., keypress,
# mouse) in the vtkRenderWindow. These events are translated into
# event invocations that VTK understands (see VTK/Common/vtkCommand.h
# for all events that VTK processes). Then observers of these VTK
# events can process them as appropriate.
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

#
# By default the vtkRenderWindowInteractor instantiates an instance
# of vtkInteractorStyle. vtkInteractorStyle translates a set of events
# it observes into operations on the camera, actors, and/or properties
# in the vtkRenderWindow associated with the vtkRenderWinodwInteractor.
# Here we specify a particular interactor style.
style = vtk.vtkInteractorStyleTrackballCamera()
iren.SetInteractorStyle(style)

#
# Unlike the previous scripts where we performed some operations and then
# exited, here we leave an event loop running. The user can use the mouse
# and keyboard to perform the operations on the scene according to the
# current interaction style.
#

#
# Initialize and start the event loop. Once the render window appears, mouse
# in the window to move the camera. The Start() method executes an event
# loop which listens to user mouse and keyboard events. Note that keypress-e
# exits the event loop. (Look in vtkInteractorStyle.h for a summary of events, or
# the appropriate Doxygen documentation.)
#
iren.Initialize()
iren.Start()
