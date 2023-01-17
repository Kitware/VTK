#!/usr/bin/env python
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkBinnedDecimation,
    vtkPointDataToCellData,
    vtkSimpleElevationFilter,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkIOLegacy import vtkPolyDataWriter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
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

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Control resolution of the test
res = 400
dim = 40

# For timing the various tests
timer = vtkTimerLog()

# Set up rendering
ren0 = vtkRenderer()
ren0.SetViewport(0,0,0.5,0.5)
ren1 = vtkRenderer()
ren1.SetViewport(0.5,0,1.0,0.5)
ren2 = vtkRenderer()
ren2.SetViewport(0,0.5,0.5,1.0)
ren3 = vtkRenderer()
ren3.SetViewport(0.5,0.5,1.0,1.0)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Link the renderers' cameras
ren1.SetActiveCamera(ren0.GetActiveCamera())
ren2.SetActiveCamera(ren0.GetActiveCamera())
ren3.SetActiveCamera(ren0.GetActiveCamera())

# Pipeline stuff. Create a sphere source at high resolution.
sphere = vtkSphereSource()
sphere.SetThetaResolution(res)
sphere.SetPhiResolution(int(res/2))
sphere.GenerateNormalsOff()

ele = vtkSimpleElevationFilter()
ele.SetInputConnection(sphere.GetOutputPort())

pd2cd = vtkPointDataToCellData()
pd2cd.SetInputConnection(ele.GetOutputPort())
pd2cd.PassPointDataOn()
pd2cd.Update()

# Test the binning decimator: each of the four point
# generation modes.
mesh0 = vtkBinnedDecimation()
mesh0.SetInputConnection(pd2cd.GetOutputPort())
mesh0.SetPointGenerationModeToUseInputPoints()
mesh0.AutoAdjustNumberOfDivisionsOn()
mesh0.SetNumberOfXDivisions(dim)
mesh0.SetNumberOfYDivisions(dim)
mesh0.SetNumberOfZDivisions(dim)
mesh0.ProducePointDataOn()
mesh0.ProduceCellDataOn()

timer.StartTimer()
mesh0.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("1) Use Input Points:")
print("\tTime: {0}".format(time))
print("\tDivisions: {0}".format(mesh0.GetNumberOfDivisions()))
print("\tNumber output triangles: {0}".format(mesh0.GetOutput().GetNumberOfCells()))
print("\tReduced by: {0}".format((1.0 - (float(mesh0.GetOutput().GetNumberOfCells()) / float(pd2cd.GetOutput().GetNumberOfCells())))))

mapper0 = vtkPolyDataMapper()
mapper0.SetInputConnection(mesh0.GetOutputPort())

actor0 = vtkActor()
actor0.SetMapper(mapper0)
actor0.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
actor0.GetProperty().SetDiffuse(.8)
actor0.GetProperty().SetSpecular(.4)
actor0.GetProperty().SetSpecularPower(30)

mesh1 = vtkBinnedDecimation()
mesh1.SetInputConnection(pd2cd.GetOutputPort())
mesh1.SetPointGenerationModeToBinPoints()
mesh1.AutoAdjustNumberOfDivisionsOff()
mesh1.SetDivisionOrigin(mesh0.GetDivisionOrigin())
mesh1.SetDivisionSpacing(mesh0.GetDivisionSpacing())
mesh1.SetNumberOfXDivisions(dim)
mesh1.SetNumberOfYDivisions(dim)
mesh1.SetNumberOfZDivisions(dim)
mesh1.AutoAdjustNumberOfDivisionsOff()
mesh1.ProducePointDataOn()
mesh1.ProduceCellDataOn()

timer.StartTimer()
mesh1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("2) Bin Points:")
print("\tTime: {0}".format(time))
print("\tDivisions: {0}".format(mesh1.GetNumberOfDivisions()))
print("\tNumber output triangles: {0}".format(mesh1.GetOutput().GetNumberOfCells()))
print("\tReduced by: {0}".format((1.0 - (float(mesh1.GetOutput().GetNumberOfCells()) / float(pd2cd.GetOutput().GetNumberOfCells())))))

mapper1 = vtkPolyDataMapper()
mapper1.SetInputConnection(mesh1.GetOutputPort())

actor1 = vtkActor()
actor1.SetMapper(mapper1)
actor1.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
actor1.GetProperty().SetDiffuse(.8)
actor1.GetProperty().SetSpecular(.4)
actor1.GetProperty().SetSpecularPower(30)

mesh2 = vtkBinnedDecimation()
mesh2.SetInputConnection(pd2cd.GetOutputPort())
mesh2.SetPointGenerationModeToBinCenters()
mesh2.AutoAdjustNumberOfDivisionsOn()
mesh2.SetNumberOfXDivisions(dim)
mesh2.SetNumberOfYDivisions(dim)
mesh2.SetNumberOfZDivisions(dim)
mesh2.ProducePointDataOn()
mesh2.ProduceCellDataOn()

timer.StartTimer()
mesh2.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("3) Bin Centers:")
print("\tTime: {0}".format(time))
print("\tDivisions: {0}".format(mesh2.GetNumberOfDivisions()))
print("\tNumber output triangles: {0}".format(mesh2.GetOutput().GetNumberOfCells()))
print("\tReduced by: {0}".format((1.0 - (float(mesh2.GetOutput().GetNumberOfCells()) / float(pd2cd.GetOutput().GetNumberOfCells())))))

mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection(mesh2.GetOutputPort())

actor2 = vtkActor()
actor2.SetMapper(mapper2)
actor2.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
actor2.GetProperty().SetDiffuse(.8)
actor2.GetProperty().SetSpecular(.4)
actor2.GetProperty().SetSpecularPower(30)

mesh3 = vtkBinnedDecimation()
mesh3.SetInputConnection(pd2cd.GetOutputPort())
mesh3.SetPointGenerationModeToBinAverages()
mesh3.AutoAdjustNumberOfDivisionsOn()
mesh3.SetNumberOfXDivisions(dim)
mesh3.SetNumberOfYDivisions(dim)
mesh3.SetNumberOfZDivisions(dim)
mesh3.AutoAdjustNumberOfDivisionsOff()
mesh3.ProducePointDataOn()
mesh3.ProduceCellDataOn()

timer.StartTimer()
mesh3.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("4) Bin Averages:")
print("\tTime: {0}".format(time))
print("\tDivisions: {0}".format(mesh3.GetNumberOfDivisions()))
print("\tNumber output triangles: {0}".format(mesh3.GetOutput().GetNumberOfCells()))
print("\tReduced by: {0}".format((1.0 - (float(mesh3.GetOutput().GetNumberOfCells()) / float(pd2cd.GetOutput().GetNumberOfCells())))))

w = vtkPolyDataWriter()
w.SetInputConnection(mesh3.GetOutputPort())
w.SetFileName("vtk.out")
#w.Write()
#exit()

mapper3 = vtkPolyDataMapper()
mapper3.SetInputConnection(mesh3.GetOutputPort())

actor3 = vtkActor()
actor3.SetMapper(mapper3)
actor3.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
actor3.GetProperty().SetDiffuse(.8)
actor3.GetProperty().SetSpecular(.4)
actor3.GetProperty().SetSpecularPower(30)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(actor0)
ren0.SetBackground(1, 1, 1)
ren1.AddActor(actor1)
ren1.SetBackground(1, 1, 1)
ren2.AddActor(actor2)
ren2.SetBackground(1, 1, 1)
ren3.AddActor(actor3)
ren3.SetBackground(1, 1, 1)

renWin.SetSize(400, 400)
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
