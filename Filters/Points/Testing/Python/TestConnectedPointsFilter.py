#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for debugging
NPts = 5000
math = vtk.vtkMath()
math.RandomSeed(31415)

# create point cloud. A bunch of random points plus some outliers
# over the six faces of the bounding box
#
points = vtk.vtkPoints()
points.SetDataTypeToFloat()
points.SetNumberOfPoints(4*NPts+3)
scalars = vtk.vtkFloatArray()
scalars.SetNumberOfTuples(4*NPts+3)
scalars.SetName("scalars")
normals = vtk.vtkFloatArray()
normals.SetNumberOfComponents(3)
normals.SetNumberOfTuples(4*NPts+3)
normals.SetName("normals")

# Create separate nearly planar regions in the (-10,10, -10,10, -2,2) space. Note one region is larger
# to create a largest region.
for i in range(0,NPts):
    x = math.Random(-8.5,0)
    y = math.Random(-8.5,-1)
    points.SetPoint(i,x,y,math.Random(-1.45,-1.4))
    if x < -4.25:
        scalars.SetValue(i,0)
    else:
        scalars.SetValue(i,1)
    if y < -4.25:
        normals.SetTuple3(i, 0,0,1)
    else:
        normals.SetTuple3(i, 0,0,-1)

for i in range(NPts,2*NPts):
    points.SetPoint(i,math.Random(-2.5,1.5),math.Random(2.5,7.5),math.Random(-0.1,0.1))
    scalars.SetValue(i,math.Random(1,2))
    normals.SetTuple3(i, 0,0,1)

for i in range(2*NPts,3*NPts):
    points.SetPoint(i,math.Random(5,9.5),math.Random(-2.5,2.5),math.Random(1.74,1.75))
    scalars.SetValue(i,math.Random(2,3))
    normals.SetTuple3(i, 0,0,1)

# Largest region
for i in range(3*NPts,4*NPts+3):
    points.SetPoint(i,math.Random(-2,2),math.Random(-2,2),math.Random(0.74,0.75))
    scalars.SetValue(i,math.Random(3,4))
    normals.SetTuple3(i, 0,0,1)

polydata = vtk.vtkPolyData()
polydata.SetPoints(points)
polydata.GetPointData().SetScalars(scalars)
polydata.GetPointData().SetNormals(normals)

# Generate normals from resulting points

# Extract all regions---------------------------------------------a
cpf = vtk.vtkConnectedPointsFilter()
cpf.SetInputData(polydata)
cpf.SetExtractionModeToAllRegions();
cpf.SetRadius(0.25);
print(cpf)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
cpf.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of regions extracted: {0}".format(cpf.GetNumberOfExtractedRegions()))
print("   Time to extract all regions: {0}".format(time))

segMapper = vtk.vtkPointGaussianMapper()
segMapper.SetInputConnection(cpf.GetOutputPort())
segMapper.EmissiveOff()
segMapper.SetScaleFactor(0.0)
segMapper.SetScalarRange(cpf.GetOutput().GetScalarRange())

segActor = vtk.vtkActor()
segActor.SetMapper(segMapper)

# Create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputData(polydata)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Now execute and extract point seeded regions-----------------------
cpf1 = vtk.vtkConnectedPointsFilter()
cpf1.SetInputData(polydata)
cpf1.SetRadius(0.25);
cpf1.SetExtractionModeToPointSeededRegions()
cpf1.AddSeed(0)
cpf1.AddSeed(2*NPts)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
cpf1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("   Time to extract point seeded regions: {0}".format(time))

segMapper1 = vtk.vtkPointGaussianMapper()
segMapper1.SetInputConnection(cpf1.GetOutputPort())
segMapper1.EmissiveOff()
segMapper1.SetScaleFactor(0.0)
segMapper1.SetScalarRange(cpf1.GetOutput().GetScalarRange())

segActor1 = vtk.vtkActor()
segActor1.SetMapper(segMapper1)

# Create an outline
outline1 = vtk.vtkOutlineFilter()
outline1.SetInputData(polydata)

outlineMapper1 = vtk.vtkPolyDataMapper()
outlineMapper1.SetInputConnection(outline1.GetOutputPort())

outlineActor1 = vtk.vtkActor()
outlineActor1.SetMapper(outlineMapper1)

# Test largest region------------------------------------------------
cpf2 = vtk.vtkConnectedPointsFilter()
cpf2.SetInputData(polydata)
cpf2.SetRadius(0.25);
cpf2.SetExtractionModeToLargestRegion()

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
cpf2.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("   Time to extract largest region: {0}".format(time))

segMapper2 = vtk.vtkPointGaussianMapper()
segMapper2.SetInputConnection(cpf2.GetOutputPort())
segMapper2.EmissiveOff()
segMapper2.SetScaleFactor(0.0)
segMapper2.SetScalarRange(cpf2.GetOutput().GetScalarRange())

segActor2 = vtk.vtkActor()
segActor2.SetMapper(segMapper2)

# Create an outline
outline2 = vtk.vtkOutlineFilter()
outline2.SetInputData(polydata)

outlineMapper2 = vtk.vtkPolyDataMapper()
outlineMapper2.SetInputConnection(outline2.GetOutputPort())

outlineActor2 = vtk.vtkActor()
outlineActor2.SetMapper(outlineMapper2)

# Test specified regions----------------------------------------------
cpf3 = vtk.vtkConnectedPointsFilter()
cpf3.SetInputData(polydata)
cpf3.SetRadius(0.25);
cpf3.SetExtractionModeToSpecifiedRegions();
cpf3.AddSpecifiedRegion(1)
cpf3.AddSpecifiedRegion(3)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
cpf3.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("   Time to extract specified regions: {0}".format(time))

segMapper3 = vtk.vtkPointGaussianMapper()
segMapper3.SetInputConnection(cpf3.GetOutputPort())
segMapper3.EmissiveOff()
segMapper3.SetScaleFactor(0.0)
segMapper3.SetScalarRange(cpf3.GetOutput().GetScalarRange())

segActor3 = vtk.vtkActor()
segActor3.SetMapper(segMapper3)

# Create an outline
outline3 = vtk.vtkOutlineFilter()
outline3.SetInputData(polydata)

outlineMapper3 = vtk.vtkPolyDataMapper()
outlineMapper3.SetInputConnection(outline3.GetOutputPort())

outlineActor3 = vtk.vtkActor()
outlineActor3.SetMapper(outlineMapper3)


# Now execute and extract all regions with scalar connectivity------------------
cpf4 = vtk.vtkConnectedPointsFilter()
cpf4.SetInputData(polydata)
cpf4.SetRadius(0.25);
cpf4.SetExtractionModeToLargestRegion()
cpf4.ScalarConnectivityOn()
cpf4.SetScalarRange(0,0.99)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
cpf4.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of regions extracted: {0}".format(cpf4.GetNumberOfExtractedRegions()))
print("   Time to extract scalar connected regions: {0}".format(time))

segMapper4 = vtk.vtkPointGaussianMapper()
segMapper4.SetInputConnection(cpf4.GetOutputPort())
segMapper4.EmissiveOff()
segMapper4.SetScaleFactor(0.0)
segMapper4.SetScalarRange(cpf4.GetOutput().GetScalarRange())

segActor4 = vtk.vtkActor()
segActor4.SetMapper(segMapper4)

# Create an outline
outline4 = vtk.vtkOutlineFilter()
outline4.SetInputData(polydata)

outlineMapper4 = vtk.vtkPolyDataMapper()
outlineMapper4.SetInputConnection(outline4.GetOutputPort())

outlineActor4 = vtk.vtkActor()
outlineActor4.SetMapper(outlineMapper4)

# Now execute and extract point seeded regions-----------------------
cpf5 = vtk.vtkConnectedPointsFilter()
cpf5.SetInputData(polydata)
cpf5.SetRadius(0.25);
cpf5.SetExtractionModeToLargestRegion()
cpf5.ScalarConnectivityOn()
cpf5.SetScalarRange(0,0.99)
cpf5.AlignedNormalsOn()
cpf5.SetNormalAngle(12.5)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
cpf5.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of regions extracted: {0}".format(cpf5.GetNumberOfExtractedRegions()))
print("   Time to extract scalar/normal connected regions: {0}".format(time))

segMapper5 = vtk.vtkPointGaussianMapper()
segMapper5.SetInputConnection(cpf5.GetOutputPort())
segMapper5.EmissiveOff()
segMapper5.SetScaleFactor(0.0)
segMapper5.SetScalarRange(cpf5.GetOutput().GetScalarRange())

segActor5 = vtk.vtkActor()
segActor5.SetMapper(segMapper5)

# Create an outline
outline5 = vtk.vtkOutlineFilter()
outline5.SetInputData(polydata)

outlineMapper5 = vtk.vtkPolyDataMapper()
outlineMapper5.SetInputConnection(outline5.GetOutputPort())

outlineActor5 = vtk.vtkActor()
outlineActor5.SetMapper(outlineMapper5)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
ren0.SetViewport(0,0,0.5,0.33)
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0.5,0,1,0.33)
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0,0.33,0.5,0.67)
ren3 = vtk.vtkRenderer()
ren3.SetViewport(0.5,0.33,1,0.67)
ren4 = vtk.vtkRenderer()
ren4.SetViewport(0,0.67,0.5,1)
ren5 = vtk.vtkRenderer()
ren5.SetViewport(0.5,0.67,1,1)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)
renWin.AddRenderer(ren5)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(segActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

ren1.AddActor(segActor1)
ren1.AddActor(outlineActor1)
ren1.SetBackground(0.1, 0.2, 0.4)

ren2.AddActor(segActor2)
ren2.AddActor(outlineActor2)
ren2.SetBackground(0.1, 0.2, 0.4)

ren3.AddActor(segActor3)
ren3.AddActor(outlineActor3)
ren3.SetBackground(0.1, 0.2, 0.4)

ren4.AddActor(segActor4)
ren4.AddActor(outlineActor4)
ren4.SetBackground(0.1, 0.2, 0.4)

ren5.AddActor(segActor5)
ren5.AddActor(outlineActor5)
ren5.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(400,600)

cam = ren0.GetActiveCamera()
ren1.SetActiveCamera(cam)
ren2.SetActiveCamera(cam)
ren3.SetActiveCamera(cam)
ren4.SetActiveCamera(cam)
ren5.SetActiveCamera(cam)
cam.SetFocalPoint(0,0,0)
cam.SetPosition(.5,.5,1)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
