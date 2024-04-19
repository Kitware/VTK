#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersPoints import (
    vtkBoundedPointSource,
    vtkExtractHierarchicalBins,
    vtkHierarchicalBinningFilter,
)
from vtkmodules.vtkFiltersSources import vtkOutlineSource
from vtkmodules.vtkIOXML import vtkXMLPolyDataWriter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPointGaussianMapper,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Interpolate onto a volume

# Parameters for debugging
NPts = 1000000
binNum = 16
math = vtkMath()
math.RandomSeed(31415)

# create pipeline
#
points = vtkBoundedPointSource()
points.SetNumberOfPoints(NPts)
points.ProduceRandomScalarsOn()
points.ProduceCellOutputOff()
points.Update()

# Bin the points
hBin = vtkHierarchicalBinningFilter()
hBin.SetInputConnection(points.GetOutputPort())
#hBin.AutomaticOn()
hBin.AutomaticOff()
hBin.SetDivisions(2,2,2)
hBin.SetBounds(points.GetOutput().GetBounds())

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
hBin.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Points processed: {0}".format(NPts))
print("   Time to bin: {0}".format(time))
#print(hBin)
#print(hBin.GetOutput())

# write stuff out
w = vtkXMLPolyDataWriter()
w.SetFileName("binPoints.vtp")
w.SetInputConnection(hBin.GetOutputPort())
#w.SetDataModeToAscii()
#w.Write()

# Output a selected bin of points
extBin = vtkExtractHierarchicalBins()
extBin.SetInputConnection(hBin.GetOutputPort())
extBin.SetBinningFilter(hBin)
extBin.SetLevel(1000)
extBin.Update() #check clamping on level number
extBin.SetBin(1000000000) # check clamping of bin number
#extBin.SetLevel(0)
extBin.SetLevel(-1)
extBin.SetBin(binNum)
extBin.Update()

subMapper = vtkPointGaussianMapper()
subMapper.SetInputConnection(extBin.GetOutputPort())
subMapper.EmissiveOff()
subMapper.SetScaleFactor(0.0)

subActor = vtkActor()
subActor.SetMapper(subMapper)

# Create an outline
outline = vtkOutlineFilter()
outline.SetInputConnection(points.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create another outline
bds = [0,0,0,0,0,0]
hBin.GetBinBounds(binNum,bds)
binOutline = vtkOutlineSource()
binOutline.SetBounds(bds)

binOutlineMapper = vtkPolyDataMapper()
binOutlineMapper.SetInputConnection(binOutline.GetOutputPort())

binOutlineActor = vtkActor()
binOutlineActor.SetMapper(binOutlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor()
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
