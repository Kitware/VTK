#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create two test cases for debugging purposes. The first
# case is a single labeled voxel. The second is eight
# labeled voxels.

# Manually create a sample image with one labeled region.
VTK_SHORT = 4
xDim = 3
yDim = 4
zDim = 5
sliceSize = xDim * yDim
numVoxels = xDim * yDim * zDim
image = vtk.vtkImageData()
image.SetDimensions(xDim,yDim,zDim)
image.AllocateScalars(VTK_SHORT,1)

imMapper = vtk.vtkDataSetMapper()
imMapper.SetInputData(image)

imActor = vtk.vtkActor()
imActor.SetMapper(imMapper)

# Fill the scalars with 0 and then set particular values.
# Here we'll create several regions / labels.
def GenIndex(i,j,k):
    return i + j*xDim + k*sliceSize
scalars = image.GetPointData().GetScalars()
scalars.Fill(0)

# Region 1
scalars.SetTuple1(GenIndex(1,1,1),1)

# Extract the boundaries of labels 1 with SurfaceNets. In this test,
# it should just produce a hex around the single labeled point. Also
# disable smoothing as it makes no sense in this situation.
snets = vtk.vtkSurfaceNets3D()
snets.SetInputData(image)
snets.SetValue(0,1)
snets.GetSmoother().SetNumberOfIterations(0)
snets.GetSmoother().SetRelaxationFactor(0.2)
snets.GetSmoother().SetConstraintDistance(0.25)

timer = vtk.vtkTimerLog()
timer.StartTimer()
snets.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to generate Surface Net: {0}".format(time))

w = vtk.vtkPolyDataWriter()
w.SetInputConnection(snets.GetOutputPort())
w.SetFileName("out.vtk")
#w.Write()

# Clipped polygons are generated
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(snets.GetOutputPort())
mapper.SetScalarModeToUseCellData()
mapper.SelectColorArray("BoundaryLabels")
mapper.SetScalarRange(1,8)

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetInterpolationToFlat()

# Manually create a sample image with one labeled region. Make
# sure the dimensions are not equal.
xDim = 4
yDim = 5
zDim = 6
sliceSize = xDim * yDim
numVoxels = xDim * yDim * zDim
image2 = vtk.vtkImageData()
image2.SetDimensions(xDim,yDim,zDim)
image2.AllocateScalars(VTK_SHORT,1)

imMapper2 = vtk.vtkDataSetMapper()
imMapper2.SetInputData(image2)

imActor2 = vtk.vtkActor()
imActor2.SetMapper(imMapper2)

# Fill the scalars with 0 and then set particular values.
# Here we'll create several regions / labels.
scalars2 = image2.GetPointData().GetScalars()
scalars2.Fill(0)

# Region 1
scalars2.SetTuple1(GenIndex(1,1,1),1)
scalars2.SetTuple1(GenIndex(2,1,1),2)
scalars2.SetTuple1(GenIndex(1,2,1),3)
scalars2.SetTuple1(GenIndex(2,2,1),4)
scalars2.SetTuple1(GenIndex(1,1,2),5)
scalars2.SetTuple1(GenIndex(2,1,2),6)
scalars2.SetTuple1(GenIndex(1,2,2),7)
scalars2.SetTuple1(GenIndex(2,2,2),8)

# Extract the boundaries of labels 1 with SurfaceNets. In this test,
# it should just produce a hex around the single labeled point. Also
# disable smoothing as it makes no sense in this situation.
snets2 = vtk.vtkSurfaceNets3D()
snets2.SetInputData(image2)
snets2.SetValue(0,1)
snets2.SetValue(1,2)
snets2.SetValue(2,3)
snets2.SetValue(3,4)
snets2.SetValue(4,5)
snets2.SetValue(5,6)
snets2.SetValue(6,7)
snets2.SetValue(7,8)
snets2.GetSmoother().SetNumberOfIterations(0)
snets2.GetSmoother().SetRelaxationFactor(0.2)
snets2.GetSmoother().SetConstraintDistance(0.25)

timer.StartTimer()
snets2.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to generate Surface Net: {0}".format(time))

# Clipped polygons are generated
mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(snets2.GetOutputPort())
mapper2.SetScalarModeToUseCellData()
mapper2.SelectColorArray("BoundaryLabels")
mapper2.SetScalarRange(1,8)

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)
actor2.GetProperty().SetInterpolationToFlat()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
ren1.SetBackground(0,0,0)
ren1.SetViewport(0,0,0.5,1)
ren2 = vtk.vtkRenderer()
ren2.SetBackground(0,0,0)
ren2.SetViewport(0.5,0,1,1)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.SetSize(400,200)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor)
ren2.AddActor(actor2)

cam = ren2.GetActiveCamera()
cam.SetPosition(-1,.9,.7)
cam.SetFocalPoint(0,0,0)
ren2.ResetCamera()
ren1.SetActiveCamera(cam)
renWin.Render()

iren.Start()
