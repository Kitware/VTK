#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
tss = vtk.vtkTexturedSphereSource()
tss.SetThetaResolution(18)
tss.SetPhiResolution(9)
earthMapper = vtk.vtkPolyDataMapper()
earthMapper.SetInputConnection(tss.GetOutputPort())
earthActor = vtk.vtkActor()
earthActor.SetMapper(earthMapper)
# load in the texture map
#
atext = vtk.vtkTexture()
pnmReader = vtk.vtkPNMReader()
pnmReader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/earth.ppm")
atext.SetInputConnection(pnmReader.GetOutputPort())
atext.InterpolateOn()
earthActor.SetTexture(atext)
# create a earth source and actor
#
es = vtk.vtkEarthSource()
es.SetRadius(0.501)
es.SetOnRatio(2)
earth2Mapper = vtk.vtkPolyDataMapper()
earth2Mapper.SetInputConnection(es.GetOutputPort())
earth2Actor = vtk.vtkActor()
earth2Actor.SetMapper(earth2Mapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(earthActor)
ren1.AddActor(earth2Actor)
ren1.SetBackground(0,0,0.1)
renWin.SetSize(300,300)
# render the image
#
ren1.ResetCamera()
cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
