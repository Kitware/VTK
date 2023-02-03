#!/usr/bin/env python
import vtk
from math import cos, sin, pi
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

math = vtk.vtkMath()

# Set the size of the test
dim = 128
numBlobs = 12

# Generate some random colors
def MakeColors (lut, n):
    lut.SetNumberOfColors(n+1)
    lut.SetTableRange(0, n)
    lut.SetScaleToLinear()
    lut.Build()
    lut.SetTableValue(0, 0, 0, 0, 1)
    math.RandomSeed(5071)
    i = 1
    while i < n:
        lut.SetTableValue(i, math.Random(.2, 1),
          math.Random(.2, 1), math.Random(.2, 1), 1)
        i += 1

lut = vtk.vtkLookupTable()
MakeColors(lut, numBlobs)
radius = 10

# Create a bunch of spherical blobs that will be segmented
# out at a later time.
blobImage = vtk.vtkImageData()

i = 0
while i < numBlobs:
    sphere = vtk.vtkSphere()
    sphere.SetRadius(radius)
    max = 50 - radius
    sphere.SetCenter(int(math.Random(-max, max)),
      int(math.Random(-max, max)), int(math.Random(-max, max)))

    sampler = vtk.vtkSampleFunction()
    sampler.SetImplicitFunction(sphere)
    sampler.SetOutputScalarTypeToFloat()
    sampler.SetSampleDimensions(dim, dim, dim)
    sampler.SetModelBounds(-50, 50, -50, 50, -50, 50)

    thres = vtk.vtkImageThreshold()
    thres.SetInputConnection(sampler.GetOutputPort())
    thres.ThresholdByLower(radius * radius)
    thres.ReplaceInOn()
    thres.ReplaceOutOn()
    thres.SetInValue(i + 1)
    thres.SetOutValue(0)
    thres.Update()
    if (i == 0):
        blobImage.DeepCopy(thres.GetOutput())

    maxValue = vtk.vtkImageMathematics()
    maxValue.SetInputData(0, blobImage)
    maxValue.SetInputData(1, thres.GetOutput())
    maxValue.SetOperationToMax()
    maxValue.Modified()
    maxValue.Update()

    blobImage.DeepCopy(maxValue.GetOutput())

    i += 1

angle = pi/6
orientation = [
  -cos(angle), 0, sin(angle),
  0, 1, 0,
  sin(angle), 0, cos(angle),
]
blobImage.SetDirectionMatrix(orientation)

# Extract labeled blobs with no smoothing
snets = vtk.vtkSurfaceNets3D()
snets.SetInputData(blobImage)
snets.GenerateLabels(numBlobs, 1, numBlobs)
snets.GetSmoother().SetNumberOfIterations(0)

timer = vtk.vtkTimerLog()
print("Start processing")
timer.StartTimer()
snets.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to generate Surface Net: {0}".format(time))

w = vtk.vtkPolyDataWriter()
w.SetInputConnection(snets.GetOutputPort())
w.SetFileName("out.vtk")
#w.Write()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(snets.GetOutputPort())
mapper.SetLookupTable(lut)
mapper.SetScalarRange(0, lut.GetNumberOfColors())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Put an outline around it
outline = vtk.vtkImageDataOutlineFilter()
outline.SetInputData(blobImage)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(1,1,1)

# Extract labeled blobs with constrained smoothing
snets2 = vtk.vtkSurfaceNets3D()
snets2.SetInputData(blobImage)
snets2.GenerateLabels(numBlobs, 1, numBlobs)
snets2.GetSmoother().SetNumberOfIterations(50)
snets2.GetSmoother().SetRelaxationFactor(0.5)
snets2.GetSmoother().SetConstraintDistance(1)
snets2.SetOutputMeshTypeToQuads()
snets2.SetOutputMeshTypeToTriangles()
snets2.SetOutputStyleToBoundary()

timer = vtk.vtkTimerLog()
timer.StartTimer()
snets2.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to generate smoothed Surface Net: {0}".format(time))

mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(snets2.GetOutputPort())
mapper2.SetLookupTable(lut)
mapper2.SetScalarRange(0, lut.GetNumberOfColors())

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)

# Put an outline around it
outline = vtk.vtkImageDataOutlineFilter()
outline.SetInputData(blobImage)

outlineMapper2 = vtk.vtkPolyDataMapper()
outlineMapper2.SetInputConnection(outline.GetOutputPort())

outlineActor2 = vtk.vtkActor()
outlineActor2.SetMapper(outlineMapper2)
outlineActor2.GetProperty().SetColor(1,1,1)

# Extract labeled blobs with windowed sync smoothing
snets3 = vtk.vtkSurfaceNets3D()
snets3.SetInputData(blobImage)
snets3.GenerateLabels(numBlobs, 1, numBlobs)
snets3.SmoothingOff()
snets3.SetOutputMeshTypeToQuads()

smoother3 = vtk.vtkWindowedSincPolyDataFilter()
smoother3.SetInputConnection(snets3.GetOutputPort())
smoother3.SetNumberOfIterations(40)
smoother3.FeatureEdgeSmoothingOff()
smoother3.BoundarySmoothingOff()
smoother3.NonManifoldSmoothingOff()

timer = vtk.vtkTimerLog()
timer.StartTimer()
smoother3.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to generate Surface Net + windowed sinc: {0}".format(time))

mapper3 = vtk.vtkPolyDataMapper()
mapper3.SetInputConnection(smoother3.GetOutputPort())
mapper3.SetLookupTable(lut)
mapper3.SetScalarRange(0, lut.GetNumberOfColors())

actor3 = vtk.vtkActor()
actor3.SetMapper(mapper3)

# Put an outline around it
outline = vtk.vtkImageDataOutlineFilter()
outline.SetInputData(blobImage)

outlineMapper3 = vtk.vtkPolyDataMapper()
outlineMapper3.SetInputConnection(outline.GetOutputPort())

outlineActor3 = vtk.vtkActor()
outlineActor3.SetMapper(outlineMapper3)
outlineActor3.GetProperty().SetColor(1,1,1)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0,0,0.33,1)
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.33,0,0.67,1)
ren3 = vtk.vtkRenderer()
ren3.SetViewport(0.67,0,1,1)

renWin = vtk.vtkRenderWindow()
renWin.SetSize(600,200)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor)
ren1.AddActor(outlineActor)
ren1.ResetCamera()

ren2.AddActor(actor2)
ren2.AddActor(outlineActor2)
ren2.SetActiveCamera(ren1.GetActiveCamera())

ren3.AddActor(actor3)
ren3.AddActor(outlineActor3)
ren3.SetActiveCamera(ren1.GetActiveCamera())

renWin.Render()
iren.Start()
