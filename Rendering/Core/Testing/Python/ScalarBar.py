#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline
#
# create sphere to color
sphere = vtk.vtkSphereSource()
sphere.SetThetaResolution(20)
sphere.SetPhiResolution(40)
def colorCells (__vtk__temp0=0,__vtk__temp1=0):
    randomColorGenerator = vtk.vtkMath()
    input = randomColors.GetInput()
    output = randomColors.GetOutput()
    numCells = input.GetNumberOfCells()
    colors = vtk.vtkFloatArray()
    colors.SetNumberOfTuples(numCells)
    i = 0
    while i < numCells:
        colors.SetValue(i,randomColorGenerator.Random(0,1))
        i = i + 1

    output.GetCellData().CopyScalarsOff()
    output.GetCellData().PassData(input.GetCellData())
    output.GetCellData().SetScalars(colors)
    del colors
    #reference counting - it's ok
    del randomColorGenerator

# Compute random scalars (colors) for each cell
randomColors = vtk.vtkProgrammableAttributeDataFilter()
randomColors.SetInputConnection(sphere.GetOutputPort())
randomColors.SetExecuteMethod(colorCells)
# mapper and actor
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(randomColors.GetOutputPort())
mapper.SetScalarRange(randomColors.GetPolyDataOutput().GetScalarRange())
sphereActor = vtk.vtkActor()
sphereActor.SetMapper(mapper)
# Create a scalar bar
scalarBar = vtk.vtkScalarBarActor()
scalarBar.SetLookupTable(mapper.GetLookupTable())
scalarBar.SetTitle("Temperature")
scalarBar.GetPositionCoordinate().SetCoordinateSystemToNormalizedViewport()
scalarBar.GetPositionCoordinate().SetValue(0.1,0.01)
scalarBar.SetOrientationToHorizontal()
scalarBar.SetWidth(0.8)
scalarBar.SetHeight(0.17)
# Test the Get/Set Position 
scalarBar.SetPosition(scalarBar.GetPosition())
# Create graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.AddActor(sphereActor)
ren1.AddActor2D(scalarBar)
renWin.SetSize(350,350)
# render the image
#
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)
renWin.Render()
scalarBar.SetNumberOfLabels(8)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
