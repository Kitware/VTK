#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
NPts = 1000000
binNum = 16
math = vtk.vtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = vtk.vtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Bin the points
hBin = vtk.vtkHierarchicalBinningFilter()
hBin.SetInputConnection(points.GetOutputPort())
#hBin.AutomaticOn()
hBin.AutomaticOff()
hBin.SetDivisions(2,2,2)
hBin.SetBounds(points.GetOutput().GetBounds())

# Time execution
timer = vtk.vtkTimerLog()
timer.StartTimer()
hBin.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to bin: {0}".format(time))
#print(hBin)
#print(hBin.GetOutput())

# write stuff out
w = vtk.vtkXMLPolyDataWriter()
w.SetFileName("binPoints.vtp")
w.SetInputConnection(hBin.GetOutputPort())
#w.SetDataModeToAscii()
#w.Write()

# Output a selected bin of points
extBin = vtk.vtkExtractHierarchicalBins()
extBin.SetInputConnection(hBin.GetOutputPort())
extBin.SetBinningFilter(hBin)
#extBin.SetLevel(0)
extBin.SetLevel(-1)
extBin.SetBin(binNum)

subMapper = vtk.vtkPointGaussianMapper()
subMapper.SetInputConnection(extBin.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = vtk.vtkActor()
subActor.SetMapper(subMapper)

# Create an outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(points.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create another outline
bds = [0,0,0,0,0,0]
hBin.GetBinBounds(binNum,bds)
binOutline = vtk.vtkOutlineSource()
binOutline.SetBounds(bds)

binOutlineMapper = vtk.vtkPolyDataMapper()
binOutlineMapper.SetInputConnection(binOutline.GetOutputPort())

binOutlineActor = vtk.vtkActor()
binOutlineActor.SetMapper(binOutlineMapper)

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
ren0.AddActor(binOutlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(250,250)

cam = ren0.GetActiveCamera()
cam.SetFocalPoint(1,1,1)
cam.SetPosition(0,0,0)
ren0.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()

iren.Start()
