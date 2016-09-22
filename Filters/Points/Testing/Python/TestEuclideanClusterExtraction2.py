#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for debugging
NPts = 100000
math = vtk.vtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = vtk.vtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.SetBounds(-3,3, -1,1, -1,1)
points.ProduceRandomScalarsOff()
points.ProduceCellOutputOff()

# create some scalars based on implicit function
# Create a cylinder
cyl = vtk.vtkCylinder()
cyl.SetCenter(-2,0,0)
cyl.SetRadius(0.02)

# Create a (thin) box implicit function
box = vtk.vtkBox()
box.SetBounds(-1,0.5, -0.5,0.5, -0.0005, 0.0005)

# Create a sphere implicit function
sphere = vtk.vtkSphere()
sphere.SetCenter(2,0,0)
sphere.SetRadius(0.8)

# Boolean (union) these together
imp = vtk.vtkImplicitBoolean()
imp.SetOperationTypeToUnion()
imp.AddFunction(cyl)
imp.AddFunction(box)
imp.AddFunction(sphere)

# Generate scalars and vector
sample = vtk.vtkSampleImplicitFunctionFilter()
sample.SetInputConnection(points.GetOutputPort())
sample.SetImplicitFunction(imp)
sample.Update()
print(sample.GetOutput().GetScalarRange())

# Now see if we can extract the three objects as separate clusters.
extr = vtk.vtkEuclideanClusterExtraction()
extr.SetInputConnection(sample.GetOutputPort())
extr.SetRadius(0.15)
#extr.ColorClustersOn()
#extr.SetExtractionModeToAllClusters()
extr.SetExtractionModeToLargestCluster()
extr.ScalarConnectivityOn()
extr.SetScalarRange(-0.64,-.3)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
extr.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(points.GetOutput().GetNumberOfPoints()))
print("   Time to segment objects: {0}".format(time))
print("   Number of clusters: {0}".format(extr.GetNumberOfExtractedClusters()))

# Draw the points
subMapper = vtk.vtkPointGaussianMapper()
subMapper.SetInputConnection(extr.GetOutputPort(0))
#subMapper.SetInputConnection(sample.GetOutputPort(0))
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)
subMapper.SetScalarRange(-0.64,2.25)

subActor = vtk.vtkActor()
subActor.SetMapper(subMapper)

# Create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(subActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(0,0,-1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

#iren.Start()
