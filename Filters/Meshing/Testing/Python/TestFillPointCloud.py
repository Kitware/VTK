#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control problem size and set debugging parameters
NPts = 1000
PointsPerBucket = 3

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create some points and display them
#
lissa = vtk.vtkLissajousPointCloud()
lissa.SetNumberOfPoints(NPts)
lissa.DeterministicNoiseOn()
lissa.Update()

lissaMapper = vtk.vtkPolyDataMapper()
lissaMapper.SetInputConnection(lissa.GetOutputPort())

lissaActor = vtk.vtkActor()
lissaActor.SetMapper(lissaMapper)
lissaActor.GetProperty().SetColor(1,1,1)
lissaActor.GetProperty().SetPointSize(2)

# Fill the points
filler = vtk.vtkFillPointCloud()
filler.SetInputConnection(lissa.GetOutputPort())
filler.JoggleOn()
filler.SetJoggleRadius(0.05)
filler.SetInLabel(1)
filler.SetBackgroundLabel(-100)
filler.SetFillStrategyToAdaptive()
filler.SetMaximumNumberOfPoints(100000)
filler.ManualLocatorControlOn()
filler.GetLocator().AutomaticOff()
filler.GetLocator().SetDivisions(60,60,60)
filler.GetPointSampler().SetDensityDistributionToExponential()
filler.GetPointSampler().SetN(1)
filler.Update()
print("Number of added points: {0}".format(filler.GetNumberOfAddedPoints()))

fillMapper = vtk.vtkPolyDataMapper()
fillMapper.SetInputConnection(filler.GetOutputPort())

fillActor = vtk.vtkActor()
fillActor.SetMapper(fillMapper)
fillActor.GetProperty().SetColor(1,1,1)
fillActor.GetProperty().SetPointSize(2)

# Tessellate them
#
voronoi = vtk.vtkVoronoiFlower3D()
voronoi.SetInputConnection(filler.GetOutputPort())
voronoi.SetGenerateCellScalarsToRandom()
voronoi.SetOutputTypeToBoundary()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(filler.GetOutput().GetNumberOfPoints()))
print("   Time to generate surface net: {0}".format(time))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreads()))
print("   Points generated: {0}".format(voronoi.GetOutput().GetNumberOfPoints()))
print("   Cells generated: {0}".format(voronoi.GetOutput().GetNumberOfCells()))

surfMapper = vtk.vtkPolyDataMapper()
surfMapper.SetInputConnection(voronoi.GetOutputPort())
surfMapper.SetScalarRange(0,64)
#surfMapper.SetScalarRange(0,voronoi.GetOutput().GetNumberOfCells())
surfMapper.SetScalarModeToUseCellData()
print("Scalar Range: {}".format(surfMapper.GetScalarRange()))
print("   Number of primitives produced: {}".format(voronoi.GetOutput().GetNumberOfCells()))

surfActor = vtk.vtkActor()
surfActor.SetMapper(surfMapper)
surfActor.GetProperty().SetColor(1,0,0)
surfActor.GetProperty().EdgeVisibilityOn()

# Highlight points
sph = vtk.vtkSphereSource()
sph.SetThetaResolution(8)
sph.SetPhiResolution(4)
sph.SetRadius(0.001)

glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(voronoi.GetOutputPort())
glyph.SetSourceConnection(sph.GetOutputPort())
glyph.ScalingOff()

ptMapper = vtk.vtkPolyDataMapper()
ptMapper.SetInputConnection(glyph.GetOutputPort())
ptMapper.SetInputConnection(filler.GetOutputPort())
ptMapper.ScalarVisibilityOff()

ptActor = vtk.vtkActor()
ptActor.SetMapper(ptMapper)
ptActor.GetProperty().SetColor(0,0,0)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(surfActor)
#ren1.AddActor(ptActor)
#ren1.AddActor(fillActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()


renWin.Render()
iren.Start()
# --- end of script --
