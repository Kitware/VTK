#!/usr/bin/env python

# create pipeline
#
reader = vtk.vtkDataSetReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/RectGrid2.vtk")
reader.Update()
warper = vtk.vtkWarpVector()
warper.SetInputConnection(reader.GetOutputPort())
warper.SetScaleFactor(0.2)
extract = vtk.vtkExtractGrid()
extract.SetInputConnection(warper.GetOutputPort())
extract.SetVOI(0,100,0,100,7,15)
mapper = vtk.vtkDataSetMapper()
mapper.SetInputConnection(extract.GetOutputPort())
mapper.SetScalarRange(0.197813,0.710419)
actor = vtk.vtkActor()
actor.SetMapper(mapper)
# Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.76213,10.712)
cam1.SetFocalPoint(-0.0842503,-0.136905,0.610234)
cam1.SetPosition(2.53813,2.2678,-5.22172)
cam1.SetViewUp(-0.241047,0.930635,0.275343)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
