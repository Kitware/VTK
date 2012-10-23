#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

#
# Demonstrate the use of clipping and capping on polyhedral data
#
# create a sphere and clip it
#
sphere = vtk.vtkSphereSource()
sphere.SetRadius(1)
sphere.SetPhiResolution(10)
sphere.SetThetaResolution(10)
plane = vtk.vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(-1,-1,0)
clipper = vtk.vtkClipPolyData()
clipper.SetInputConnection(sphere.GetOutputPort())
clipper.SetClipFunction(plane)
clipper.GenerateClipScalarsOn()
clipper.GenerateClippedOutputOn()
clipper.SetValue(0)
clipMapper = vtk.vtkPolyDataMapper()
clipMapper.SetInputConnection(clipper.GetOutputPort())
clipMapper.ScalarVisibilityOff()
backProp = vtk.vtkProperty()
backProp.SetDiffuseColor(tomato)
clipActor = vtk.vtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(peacock)
clipActor.SetBackfaceProperty(backProp)
# now extract feature edges
boundaryEdges = vtk.vtkFeatureEdges()
boundaryEdges.SetInputConnection(clipper.GetOutputPort())
boundaryEdges.BoundaryEdgesOn()
boundaryEdges.FeatureEdgesOff()
boundaryEdges.NonManifoldEdgesOff()
boundaryClean = vtk.vtkCleanPolyData()
boundaryClean.SetInputConnection(boundaryEdges.GetOutputPort())
boundaryStrips = vtk.vtkStripper()
boundaryStrips.SetInputConnection(boundaryClean.GetOutputPort())
boundaryStrips.Update()
boundaryPoly = vtk.vtkPolyData()
boundaryPoly.SetPoints(boundaryStrips.GetOutput().GetPoints())
boundaryPoly.SetPolys(boundaryStrips.GetOutput().GetLines())
boundaryTriangles = vtk.vtkTriangleFilter()
boundaryTriangles.SetInputData(boundaryPoly)
boundaryMapper = vtk.vtkPolyDataMapper()
boundaryMapper.SetInputConnection(boundaryTriangles.GetOutputPort())
boundaryActor = vtk.vtkActor()
boundaryActor.SetMapper(boundaryMapper)
boundaryActor.GetProperty().SetColor(banana)
# Create graphics stuff
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(clipActor)
ren1.AddActor(boundaryActor)
ren1.SetBackground(1,1,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(30)
ren1.GetActiveCamera().Dolly(1.2)
ren1.ResetCameraClippingRange()
renWin.SetSize(300,300)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
