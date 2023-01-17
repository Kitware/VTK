#!/usr/bin/env python
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersPoints import (
    vtkSPHInterpolator,
    vtkSPHQuinticKernel,
)
from vtkmodules.vtkFiltersSources import vtkPlaneSource
from vtkmodules.vtkIOXML import vtkXMLUnstructuredGridReader
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
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Parameters for testing
res = 250

# Graphics stuff
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
reader = vtkXMLUnstructuredGridReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/SPH_Points2D.vtu")
reader.Update()
output = reader.GetOutput()
scalarRange = output.GetPointData().GetArray("Rho").GetRange()
print("Scalar range: {}".format(scalarRange))

# Something to sample with
center = output.GetCenter()
bounds = output.GetBounds()
length = output.GetLength()

plane = vtkPlaneSource()
plane.SetResolution(res,res)
plane.SetOrigin(bounds[0],bounds[2],bounds[4])
plane.SetPoint1(bounds[1],bounds[2],bounds[4])
plane.SetPoint2(bounds[0],bounds[3],bounds[4])
plane.SetCenter(center)
plane.SetNormal(0,0,1)
plane.Update()

timer = vtkTimerLog()

# SPH kernel------------------------------------------------
sphKernel = vtkSPHQuinticKernel()
sphKernel.SetSpatialStep(0.00002)
sphKernel.SetDimension(2)

interpolator = vtkSPHInterpolator()
interpolator.SetInputConnection(plane.GetOutputPort())
interpolator.SetSourceConnection(reader.GetOutputPort())
interpolator.SetKernel(sphKernel)
interpolator.Update()

# Time execution
timer.StartTimer()
interpolator.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (SPH): {0}".format(time))

intMapper = vtkPolyDataMapper()
intMapper.SetInputConnection(interpolator.GetOutputPort())
intMapper.SetScalarModeToUsePointFieldData()
intMapper.SelectColorArray("Rho")
intMapper.SetScalarRange(750,1050)

intActor = vtkActor()
intActor.SetMapper(intMapper)

# Create an outline
outline = vtkOutlineFilter()
outline.SetInputData(output)

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

ren0.AddActor(intActor)
ren0.AddActor(outlineActor)
ren0.SetBackground(0.1, 0.2, 0.4)

iren.Initialize()
renWin.Render()

iren.Start()
