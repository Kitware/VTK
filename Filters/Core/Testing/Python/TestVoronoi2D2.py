#!/usr/bin/env python
import vtk
import math
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def uniformFn(NPts,points):
    math = vtk.vtkMath()
    math.RandomSeed(27183)
    for i in range(0,NPts):
        points.SetPoint(i,math.Random(0,1),math.Random(0,1),0.0)

def lissajousFn(A, B, a, b, delta, npts, noise):
    import numpy as np
    t = np.arange(0.0, 2.0*math.pi, 2.0*math.pi/npts);
    if noise > 0.0:
        x = A * np.sin(a*t + delta)
        y = B * np.sin(b*t)
        dx = A*a*np.cos(a*t + delta)
        dy = B*b*np.cos(b*t)
        ds2 = dx*dx + dy*dy
        ds = np.sqrt(ds2)
        x += np.random.uniform(-noise, noise, dx.size) * dy/ds
        y -= np.random.uniform(-noise, noise, dy.size) * dx/ds
    else:
        x = A * np.sin(a*t + delta)
        y = B * np.sin(b*t)
    print('First and last equal?', x[0] == x[npts-1], y[0] == y[npts-1])
    return [x, y]

def quarterDiskFn(NPts,points):
    alpha = (math.pi / 2.0) / (NPts-1)
    for i in range(0,NPts):
        points.SetPoint(i, math.cos(alpha * i), math.sin(alpha * i) ,0.0)

def KuzminFn(NPts,points):
    import numpy as np
    for i in range(0,NPts):
        angle = math.pi * 2.0 * np.random.uniform(0,1)
        length = np.random.uniform(0,1) * 0.5
        r = (1.0 - (1. / math.sqrt(1. - length*length)))
        px = math.cos(angle) * length
        py = math.sin(angle) * length
        points.SetPoint(i, px*r, py*r, 0.0)

# Control problem size and set debugging parameters
NPts = 1000
#NPts = 100000
#NPts = 1000000
MaxTileClips = NPts
PointsPerBucket = 2
GenerateFlower = 1
PointOfInterest = -1

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create some points and display them
mode = 'uniform'
# mode = 'lissajous'
# mode = 'quarterDisk'
# mode = 'Kuzmin'

points = vtk.vtkPoints()
points.SetNumberOfPoints(NPts)

if mode == 'uniform':
    uniformFn(NPts,points)
elif mode == 'lissajous':
    lf = lissajousFn(1,1,1,2,math.pi/2, NPts, noise=0.0)
    for i in range(NPts):
        points.SetPoint(i, lf[0][i], lf[1][i], 0.0)
    # Add in some background noise? For numerical studies...
    # i = 0
    # while i < NPts/5:
    #     points.InsertNextPoint(math.Random(-1,1),math.Random(-1,1),0.0)
    #     i = i + 1
    # print('Points ', points.GetNumberOfPoints())
elif mode == 'quarterDisk':
    quarterDiskFn(NPts,points)
elif mode == 'Kuzmin':
    KuzminFn(NPts,points)

profile = vtk.vtkPolyData()
profile.SetPoints(points)

ptMapper = vtk.vtkPointGaussianMapper()
ptMapper.SetInputData(profile)
ptMapper.EmissiveOff()
ptMapper.SetScaleFactor(0.0)

ptActor = vtk.vtkActor()
ptActor.SetMapper(ptMapper)
ptActor.GetProperty().SetColor(0,0,0)
ptActor.GetProperty().SetPointSize(2)

# Tessellate them
#
voronoi = vtk.vtkVoronoi2D()
voronoi.SetInputData(profile)
voronoi.SetGenerateScalarsToNone()
voronoi.SetGenerateScalarsToThreadIds()
voronoi.SetGenerateScalarsToPointIds()
voronoi.SetPointOfInterest(PointOfInterest)
voronoi.SetMaximumNumberOfTileClips(MaxTileClips)
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetGenerateVoronoiFlower(GenerateFlower)

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Voronoi tessellation: {0}".format(time))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreadsUsed()))

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(voronoi.GetOutputPort())
if voronoi.GetGenerateScalars() == 1:
    mapper.SetScalarRange(0,NPts)
elif voronoi.GetGenerateScalars() == 2:
    mapper.SetScalarRange(0,voronoi.GetNumberOfThreadsUsed())
print("Scalar Range: {}".format(mapper.GetScalarRange()))

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,0,0)

# Debug code
sphere = vtk.vtkSphereSource()
sphere.SetRadius(0.05)
sphere.SetThetaResolution(16)
sphere.SetPhiResolution(8)
if PointOfInterest >= 0:
    sphere.SetCenter(points.GetPoint(PointOfInterest))

sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.GetProperty().SetColor(0,0,0)

# Voronoi flower
fMapper = vtk.vtkPointGaussianMapper()
fMapper.SetInputConnection(voronoi.GetOutputPort(1))
fMapper.EmissiveOff()
fMapper.SetScaleFactor(0.0)

fActor = vtk.vtkActor()
fActor.SetMapper(fMapper)
fActor.GetProperty().SetColor(0,0,1)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(ptActor)
if PointOfInterest >= 0:
    ren1.AddActor(sphereActor)
    if GenerateFlower > 0:
         ren1.AddActor(fActor)
         ren1.RemoveActor(ptActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(300,300)
renWin.Render()
cam1 = ren1.GetActiveCamera()
if PointOfInterest >= 0:
    xyz = []
    xyz = points.GetPoint(PointOfInterest)
    cam1.SetFocalPoint(xyz)
    cam1.SetPosition(xyz[0],xyz[1],xyz[2]+1)
    renWin.Render()
    ren1.ResetCameraClippingRange()
    cam1.Zoom(1.5)

# added these unused default arguments so that the prototype
# matches as required in python.
def reportPointId (a=0,b=0,__vtk__temp0=0,__vtk__temp1=0):
    print("Point Id: {}".format(picker.GetPointId()))

picker = vtk.vtkPointPicker()
picker.AddObserver("EndPickEvent",reportPointId)
picker.PickFromListOn()
picker.AddPickList(ptActor)
iren.SetPicker(picker)

renWin.Render()
iren.Start()
# --- end of script --
