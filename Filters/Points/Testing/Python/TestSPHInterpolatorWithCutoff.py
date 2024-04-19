#!/usr/bin/env python
try:
    import numpy as np
except ImportError:
    print("This test requires numpy!")
    from vtkmodules.test import Testing
    Testing.skip()

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
from vtkmodules.numpy_interface import dataset_adapter as dsa
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
reader.SetFileName(VTK_DATA_ROOT + "/Data/SPH_Points.vtu")
reader.Update()
output = reader.GetOutput()
scalarRange = output.GetPointData().GetArray("Rho").GetRange()

output2 = dsa.WrapDataObject(output)
Mass = np.ones(output.GetNumberOfPoints()) * 1.0;
output2.PointData.append(Mass, "Mass")

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
plane.Push(1.15)
plane.Update()
planeOutput = plane.GetOutput()
planeOutput2 = dsa.WrapDataObject(planeOutput)

# The constant value in the Cutoff array below is equal to
# the spatialStep (0.1) set below, and the CutoffFactor (3) of the QuinticKernel.
# Note that the cutoff array should be associated with the input
# sampling points.
Cutoff = np.ones(planeOutput.GetNumberOfPoints()) * 3.0/10.0;
planeOutput2.PointData.append(Cutoff, "Cutoff")

# SPH kernel------------------------------------------------

sphKernel = vtkSPHQuinticKernel()
sphKernel.SetSpatialStep(0.1)

interpolator = vtkSPHInterpolator()
interpolator.SetInputConnection(plane.GetOutputPort())
interpolator.SetSourceConnection(reader.GetOutputPort())
interpolator.SetDensityArrayName("Rho")
interpolator.SetMassArrayName("Mass")
interpolator.SetCutoffArrayName("Cutoff")
interpolator.SetKernel(sphKernel)

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
interpolator.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Interpolate Points (SPH): {0}".format(time))
intMapper = vtkPolyDataMapper()
intMapper.SetInputConnection(interpolator.GetOutputPort())
intMapper.SetScalarModeToUsePointFieldData()
intMapper.SelectColorArray("Rho")
intMapper.SetScalarRange(interpolator.GetOutput().GetPointData().GetArray("Rho").GetRange())

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
