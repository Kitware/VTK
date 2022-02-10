#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Manually create a sample image.
VTK_SHORT = 4
rowLen = 10
image = vtk.vtkImageData()
image.SetDimensions(rowLen,7,1)
image.AllocateScalars(VTK_SHORT,1)

imMapper = vtk.vtkDataSetMapper()
imMapper.SetInputData(image)

imActor = vtk.vtkActor()
imActor.SetMapper(imMapper)

# Fill the scalars with 0 and then set particular values.
# Here we'll create several regions / labels.
def GenIndex(i,j):
    return i + j*rowLen

scalars = image.GetPointData().GetScalars()
scalars.Fill(0)
# Region 1
scalars.SetTuple1(GenIndex(0,2),1)
scalars.SetTuple1(GenIndex(1,2),1)
scalars.SetTuple1(GenIndex(2,2),1)
scalars.SetTuple1(GenIndex(3,0),1)
scalars.SetTuple1(GenIndex(3,1),1)
scalars.SetTuple1(GenIndex(3,2),1)
scalars.SetTuple1(GenIndex(3,3),1)
scalars.SetTuple1(GenIndex(4,1),1)
scalars.SetTuple1(GenIndex(3,2),1)
scalars.SetTuple1(GenIndex(4,2),1)
scalars.SetTuple1(GenIndex(4,3),1)
# Region 2
scalars.SetTuple1(GenIndex(5,2),2)
scalars.SetTuple1(GenIndex(6,2),2)
scalars.SetTuple1(GenIndex(5,3),2)
# Region 3
scalars.SetTuple1(GenIndex(3,4),3)
scalars.SetTuple1(GenIndex(4,4),3)
scalars.SetTuple1(GenIndex(4,5),3)
# Region 4
scalars.SetTuple1(GenIndex(5,4),4)
scalars.SetTuple1(GenIndex(6,4),4)
scalars.SetTuple1(GenIndex(5,5),4)
scalars.SetTuple1(GenIndex(6,5),4)
scalars.SetTuple1(GenIndex(7,5),4)
scalars.SetTuple1(GenIndex(8,5),4)
scalars.SetTuple1(GenIndex(7,6),4)
scalars.SetTuple1(GenIndex(8,6),4)
scalars.SetTuple1(GenIndex(9,6),4)

# Extract the boundaries of labels 1-4 with SurfaceNets.
# Disable smoothing.
snets = vtk.vtkSurfaceNets2D()
snets.SetInputData(image)
snets.SetValue(0,1)
snets.SetValue(1,2)
snets.SetValue(2,3)
snets.SetValue(3,4)
snets.GetSmoother().SetNumberOfIterations(0)
snets.GetSmoother().SetRelaxationFactor(0.2)
snets.GetSmoother().SetConstraintDistance(0.25)
snets.ComputeScalarsOff()

timer = vtk.vtkTimerLog()
timer.StartTimer()
snets.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to generate Surface Net: {0}".format(time))

# Clipped polygons are generated
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(snets.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
ren1.SetBackground(0,0,0)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(actor)
#ren1.AddActor(imActor) #uncomment to see image

renWin.Render()
iren.Start()
