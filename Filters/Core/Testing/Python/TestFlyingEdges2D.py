
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
xSze = 5
ySze = 12
xSze = 4000
ySze = 6000
isoValue = 750
maxValue = 100
imageData = vtk.vtkImageData()
imageData.SetDimensions(xSze,ySze,1)
scalars = vtk.vtkUnsignedCharArray()
scalars.SetNumberOfTuples(xSze*ySze)
for i in range (0,xSze*ySze):
  scalars.SetValue(i,0)
scalars.SetValue(9,maxValue)
scalars.SetValue(17,maxValue)
scalars.SetValue(18,maxValue)
scalars.SetValue(36,maxValue)
scalars.SetValue(42,maxValue)
scalars.SetValue(1000000,maxValue)
imageData.GetPointData().SetScalars(scalars)

# pipeline
reader = vtk.vtkStructuredPointsReader()
reader.SetFileName("C:\D\gitVTK\Bugs\FlyingEdges\StructuredPoints-Test.vtk")

reader2 = vtk.vtkPNGReader()
reader2.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")

iso = vtk.vtkFlyingEdges2D()
iso.SetInputData(imageData)
#iso.SetInputConnection(reader2.GetOutputPort())
iso.SetValue(0, isoValue)
#iso.GenerateValues(12,500,1150)

iso2 = vtk.vtkSynchronizedTemplates2D()
iso2.SetInputData(imageData)
#iso2.SetInputConnection(reader2.GetOutputPort())
iso2.SetValue(0, isoValue)
#iso2.GenerateValues(12,500,1150)

isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
#isoMapper.SetInputConnection(iso2.GetOutputPort())
isoMapper.ScalarVisibilityOff()
isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,1,1)

outline = vtk.vtkOutlineFilter()
outline.SetInputData(imageData)
#outline.SetInputConnection(reader2.GetOutputPort())
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
writer.Write()

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
