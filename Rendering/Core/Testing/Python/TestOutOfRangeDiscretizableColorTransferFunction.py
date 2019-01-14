#!/usr/bin/env python
import sys
import vtk

useBelowRangeColor = 0
if sys.argv.count("--useBelowRangeColor") > 0:
    useBelowRangeColor = 1
useAboveRangeColor = 0
if sys.argv.count("--useAboveRangeColor") > 0:
    useAboveRangeColor = 1


cmap = vtk.vtkDiscretizableColorTransferFunction()
cmap.AddRGBPoint(-.4, 0.8, 0.8, 0.8)
cmap.AddRGBPoint(0.4, 1, 0, 0)
cmap.SetUseBelowRangeColor(useBelowRangeColor)
cmap.SetBelowRangeColor(0.0, 1.0, 0.0)
cmap.SetUseAboveRangeColor(useAboveRangeColor)
cmap.SetAboveRangeColor(1.0, 1.0, 0.0)

sphere = vtk.vtkSphereSource()
sphere.SetPhiResolution(32)
sphere.SetThetaResolution(32)
sphere.Update()

pd = sphere.GetOutput().GetPointData()
for i in range(pd.GetNumberOfArrays()):
    print(pd.GetArray(i).GetName())

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(sphere.GetOutputPort())
mapper.SetScalarModeToUsePointFieldData()
mapper.SelectColorArray("Normals")
mapper.ColorByArrayComponent("Normals", 0)
mapper.SetLookupTable(cmap)
mapper.UseLookupTableScalarRangeOn()
mapper.InterpolateScalarsBeforeMappingOn()

actor = vtk.vtkActor()
actor.SetMapper(mapper)

renderer = vtk.vtkRenderer()
renderer.AddActor(actor)
renderer.ResetCamera()
renderer.ResetCameraClippingRange()

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(renderer)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
renWin.Render()
