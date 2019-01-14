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

# Generate some random points and scalars
points = vtk.vtkPoints()
points.SetNumberOfPoints(numPts);

da = points.GetData()
pool = vtk.vtkRandomPool()
pool.PopulateDataArray(da, 0, 2.25, 7)
pool.PopulateDataArray(da, 1, 1, 10)
pool.PopulateDataArray(da, 2, 0.5, 10.5)

scalars = vtk.vtkFloatArray()
scalars.SetNumberOfTuples(numPts)
scalars.SetName("Random Scalars");
pool.PopulateDataArray(scalars, 100,200)

profile = vtk.vtkPolyData()
profile.SetPoints(points)
profile.GetPointData().SetScalars(scalars);

extract = vtk.vtkExtractEnclosedPoints()
extract.SetInputData(profile)
extract.SetSurfaceConnection(ss.GetOutputPort())

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
extract.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to extract points: {0}".format(time))

glyph = vtk.vtkSphereSource()
glypher = vtk.vtkGlyph3D()
glypher.SetInputConnection(extract.GetOutputPort())
#glypher.SetInputConnection(extract.GetOutputPort(1))
glypher.SetSourceConnection(glyph.GetOutputPort())
glypher.SetScaleModeToDataScalingOff()
glypher.SetScaleFactor(0.25)

pointsMapper = vtk.vtkPolyDataMapper()
pointsMapper.SetInputConnection(glypher.GetOutputPort())
pointsMapper.SetScalarRange(100,200)

pointsActor = vtk.vtkActor()
pointsActor.SetMapper(pointsMapper)

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
