#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control test size
numPts = 500

# Create the RenderWindow, Renderer and both Actors
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create an enclosing surface
ss = vtk.vtkSphereSource()
ss.SetPhiResolution(25)
ss.SetThetaResolution(38)
ss.SetCenter(4.5, 5.5, 5.0)
ss.SetRadius(2.5)

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(ss.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToWireframe()

# Generate some random points
math = vtk.vtkMath()
math.RandomSeed(1177)

points = vtk.vtkPoints()
for i in range(0,numPts):
  x = math.Random(2.25,7.0)
  y = math.Random(1,10)
  z = math.Random(0.5,10.5)
  points.InsertPoint(i,x,y,z)

points.SetPoint(0,4.5,5.5,5.0)
profile = vtk.vtkPolyData()
profile.SetPoints(points)

extract = vtk.vtkExtractEnclosedPoints()
extract.SetInputData(profile)
extract.SetSurfaceConnection(ss.GetOutputPort())
extract.GenerateOutliersOn()

glyph = vtk.vtkSphereSource()
glypher = vtk.vtkGlyph3D()
glypher.SetInputConnection(extract.GetOutputPort())
#glypher.SetInputConnection(extract.GetOutputPort(1))
glypher.SetSourceConnection(glyph.GetOutputPort())
glypher.SetScaleFactor(0.25)

pointsMapper = vtk.vtkPolyDataMapper()
pointsMapper.SetInputConnection(glypher.GetOutputPort())

pointsActor = vtk.vtkActor()
pointsActor.SetMapper(pointsMapper)
pointsActor.GetProperty().SetColor(1,0,0)

# Add actors
ren.AddActor(actor)
ren.AddActor(pointsActor)

# Standard testing code.
renWin.SetSize(300,300)
renWin.Render()

# render the image
#
renWin.Render()
iren.Start()
