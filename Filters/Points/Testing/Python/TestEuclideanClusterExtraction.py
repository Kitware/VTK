#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline
#

# Create a cylinder
cyl = vtk.vtkCylinderSource()
cyl.SetCenter(-2,0,0)
cyl.SetRadius(0.02)
cyl.SetHeight(1.8)
cyl.SetResolution(24)

# Create a (thin) box implicit function
plane = vtk.vtkPlaneSource()
plane.SetOrigin(-1, -0.5, 0)
plane.SetPoint1(0.5, -0.5, 0)
plane.SetPoint2(-1, 0.5, 0)

# Create a sphere implicit function
sphere = vtk.vtkSphereSource()
sphere.SetCenter(2,0,0)
sphere.SetRadius(0.8)
sphere.SetThetaResolution(96)
sphere.SetPhiResolution(48)

# Boolean (union) these together
append = vtk.vtkAppendPolyData()
append.AddInputConnection(cyl.GetOutputPort())
append.AddInputConnection(plane.GetOutputPort())
append.AddInputConnection(sphere.GetOutputPort())

# Extract points along sphere surface
pts = vtk.vtkPolyDataPointSampler()
pts.SetInputConnection(append.GetOutputPort())
pts.SetDistance(0.025)
pts.Update()

# Now see if we can extract the three objects as separate clusters.
extr = vtk.vtkEuclideanClusterExtraction()
extr.SetInputConnection(pts.GetOutputPort())
extr.SetRadius(0.1)
extr.ColorClustersOn()
extr.SetExtractionModeToAllClusters()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
extr.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(pts.GetOutput().GetNumberOfPoints()))
print("   Time to segment objects: {0}".format(time))
print("   Number of clusters: {0}".format(extr.GetNumberOfExtractedClusters()))

# Three different outputs for different curvatures
subMapper = vtk.vtkPointGaussianMapper()
subMapper.SetInputConnection(extr.GetOutputPort(0))
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)
subMapper.SetScalarRange(0,2)

subActor = vtk.vtkActor()
subActor.SetMapper(subMapper)
subActor.AddPosition(0,2.25,0)

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
