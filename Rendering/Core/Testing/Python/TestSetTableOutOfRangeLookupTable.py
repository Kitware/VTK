#!/usr/bin/env python
import sys
from vtkmodules.vtkCommonCore import (
    vtkLookupTable,
    vtkUnsignedCharArray,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
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

useBelowRangeColor = 0
if sys.argv.count("--useBelowRangeColor") > 0:
    useBelowRangeColor = 1
useAboveRangeColor = 0
if sys.argv.count("--useAboveRangeColor") > 0:
    useAboveRangeColor = 1

colors = vtkUnsignedCharArray()
colors.SetNumberOfComponents(4)
colors.SetNumberOfTuples(256)
for i in range(256):
    colors.SetTuple(i, (i, i, i, 255))

table = vtkLookupTable()
table.SetNumberOfColors(256)
table.SetRange(-0.5, 0.5)
table.SetTable(colors)
table.SetBelowRangeColor(1, 0, 0, 1)
table.SetAboveRangeColor(0, 1, 0, 1)
table.SetUseBelowRangeColor(useBelowRangeColor)
table.SetUseAboveRangeColor(useAboveRangeColor)

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
mapper.SetLookupTable(table)
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
