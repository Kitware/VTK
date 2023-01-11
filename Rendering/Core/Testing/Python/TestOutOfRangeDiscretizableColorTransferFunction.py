#!/usr/bin/env python
import sys
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDiscretizableColorTransferFunction,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

useBelowRangeColor = 0
if sys.argv.count("--useBelowRangeColor") > 0:
    useBelowRangeColor = 1
useAboveRangeColor = 0
if sys.argv.count("--useAboveRangeColor") > 0:
    useAboveRangeColor = 1


cmap = vtkDiscretizableColorTransferFunction()
cmap.AddRGBPoint(-.4, 0.8, 0.8, 0.8)
cmap.AddRGBPoint(0.4, 1, 0, 0)
cmap.SetUseBelowRangeColor(useBelowRangeColor)
cmap.SetBelowRangeColor(0.0, 1.0, 0.0)
cmap.SetUseAboveRangeColor(useAboveRangeColor)
cmap.SetAboveRangeColor(1.0, 1.0, 0.0)

sphere = vtkSphereSource()
sphere.SetPhiResolution(32)
sphere.SetThetaResolution(32)
sphere.Update()

pd = sphere.GetOutput().GetPointData()
for i in range(pd.GetNumberOfArrays()):
    print(pd.GetArray(i).GetName())

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(sphere.GetOutputPort())
mapper.SetScalarModeToUsePointFieldData()
mapper.SelectColorArray("Normals")
mapper.ColorByArrayComponent("Normals", 0)
mapper.SetLookupTable(cmap)
mapper.UseLookupTableScalarRangeOn()
mapper.InterpolateScalarsBeforeMappingOn()

actor = vtkActor()
actor.SetMapper(mapper)

renderer = vtkRenderer()
renderer.AddActor(actor)
renderer.ResetCamera()
renderer.ResetCameraClippingRange()

renWin = vtkRenderWindow()
renWin.AddRenderer(renderer)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
renWin.Render()
