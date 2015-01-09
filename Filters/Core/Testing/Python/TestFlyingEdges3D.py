
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create image manually for testing purposes
xSze = 6
ySze = 5
zSze = 8
#xSze = 150
#ySze = 175
#zSze = 750
isoValue = 200
maxValue = 250
imageData = vtk.vtkImageData()
imageData.SetDimensions(xSze,ySze,zSze)
scalars = vtk.vtkIntArray()
scalars.SetNumberOfTuples(xSze*ySze*zSze)
for i in range (0,xSze*ySze*zSze):
#  scalars.SetValue(i,i)
  scalars.SetValue(i,0)
scalars.SetValue(222,maxValue)
imageData.GetPointData().SetScalars(scalars)

# pipeline
reader = vtk.vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)

# another source
sphere = vtk.vtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

# iso-surface to create geometry
sample = vtk.vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(200,200,200)

iso = vtk.vtkFlyingEdges3D()
#iso.SetInputData(imageData)
#iso.SetInputConnection(reader.GetOutputPort())
iso.SetInputConnection(sample.GetOutputPort())
#iso.SetValue(0, isoValue)
#iso.SetValue(0, 750)
#iso.SetValue(0, 0.0)
#iso.GenerateValues(12,500,1150)
iso.GenerateValues(3,-.11,.11)
iso.ComputeNormalsOn()
iso.ComputeGradientsOff()

iso2 = vtk.vtkSynchronizedTemplates3D()
#iso2.SetInputData(imageData)
#iso2.SetInputConnection(reader.GetOutputPort())
iso2.SetInputConnection(sample.GetOutputPort())
#iso2.SetValue(0, isoValue)
#iso2.SetValue(0, 750)
#iso2.SetValue(0, 0.0)
#iso2.GenerateValues(12,500,1150)
iso2.GenerateValues(3,-.11,.11)

isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
#isoMapper.SetInputConnection(iso2.GetOutputPort())
isoMapper.ScalarVisibilityOff()
isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,1,1)
isoActor.GetProperty().SetOpacity(0.5)

outline = vtk.vtkOutlineFilter()
#outline.SetInputData(imageData)
#outline.SetInputConnection(reader.GetOutputPort())
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()
#eval $outlineProp SetColor 0 0 0

# Time the execution of the filter
timer = vtk.vtkExecutionTimer()
timer.SetFilter(iso)
iso.Update()
FEwallClock = timer.GetElapsedWallClockTime()
FEcpuClock = timer.GetElapsedCPUTime()
print ("FE:", FEwallClock, FEcpuClock)

timer2 = vtk.vtkExecutionTimer()
timer2.SetFilter(iso2)
iso2.Update()
wallClock = timer2.GetElapsedWallClockTime()
cpuClock = timer2.GetElapsedCPUTime()
print ("ST:", wallClock, cpuClock)

if FEwallClock > 0:
  print ("Speedup:", (wallClock/FEwallClock))

#write output
writer = vtk.vtkPolyDataWriter()
writer.SetInputConnection(iso.GetOutputPort())
writer.SetFileName("C:\D\gitVTK\Bugs\FlyingEdges\FE-output.vtk")
#if xSze*ySze <= 100:
#writer.Write()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(400,400)
ren1.ResetCamera()
iren.Initialize()

iren.Start()
# prevent the tk window from showing up then start the event loop
# --- end of script --
