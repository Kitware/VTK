#!/usr/bin/env python

# Test the quadric decimation LOD actor
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# pipeline stuff
#
sphere = vtk.vtkSphereSource()
sphere.SetPhiResolution(150)
sphere.SetThetaResolution(150)
plane = vtk.vtkPlaneSource()
plane.SetXResolution(150)
plane.SetYResolution(150)
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(sphere.GetOutputPort())
mapper.SetInputConnection(plane.GetOutputPort())
actor = vtk.vtkQuadricLODActor()
actor.SetMapper(mapper)
actor.DeferLODConstructionOff()
actor.GetProperty().SetRepresentationToWireframe()
actor.GetProperty().SetDiffuseColor(tomato)
actor.GetProperty().SetDiffuse(.8)
actor.GetProperty().SetSpecular(.4)
actor.GetProperty().SetSpecularPower(30)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
