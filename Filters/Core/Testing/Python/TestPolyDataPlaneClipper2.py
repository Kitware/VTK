#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkFiltersCore import (
    vtkAppendPolyData,
    vtkPointDataToCellData,
    vtkPolyDataPlaneClipper,
    vtkSimpleElevationFilter,
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
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the plane clipper on cell data, point data,
# and when multiple capping loops are involved.

# Control test size
res = 512

# Create the RenderWindow, Renderers and both Actors
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create some spheres
sphere1 = vtkSphereSource()
sphere1.SetCenter(0.0, 0.0, 0.0)
sphere1.SetRadius(0.25)
sphere1.SetThetaResolution(2*res)
sphere1.SetPhiResolution(res)
sphere1.Update()

ele1 = vtkSimpleElevationFilter()
ele1.SetInputConnection(sphere1.GetOutputPort())

sphere2 = vtkSphereSource()
sphere2.SetCenter(0.875, 0.0, 0.0)
sphere2.SetRadius(0.35)
sphere2.SetThetaResolution(2*res)
sphere2.SetPhiResolution(res)
sphere2.Update()

sphere3 = vtkSphereSource()
sphere3.SetCenter(2.0, 0.0, 0.0)
sphere3.SetRadius(0.5)
sphere3.SetThetaResolution(2*res)
sphere3.SetPhiResolution(res)
sphere3.Update()

appendF = vtkAppendPolyData()
appendF.AddInputConnection(sphere2.GetOutputPort())
appendF.AddInputConnection(sphere3.GetOutputPort())

ele2 = vtkSimpleElevationFilter()
ele2.SetInputConnection(appendF.GetOutputPort())

pd2cd = vtkPointDataToCellData()
pd2cd.SetInputConnection(ele2.GetOutputPort())
pd2cd.ProcessAllArraysOff()
pd2cd.AddPointDataArray("Elevation")
pd2cd.Update()

# The cut plane
plane = vtkPlane()
plane.SetOrigin(0, 0, 0)
plane.SetNormal(0, 0, -1)

# Clipper - single sphere with point data
clipper1 = vtkPolyDataPlaneClipper()
clipper1.SetInputConnection(ele1.GetOutputPort())
clipper1.SetPlane(plane)
clipper1.SetBatchSize(10000)
clipper1.CappingOn()
clipper1.Update()

# Display the clipped cells
mapper1 = vtkPolyDataMapper()
mapper1.SetInputConnection(clipper1.GetOutputPort())
mapper1.SetScalarRange(clipper1.GetOutput().GetPointData().GetScalars().GetRange())

actor1 = vtkActor()
actor1.SetMapper(mapper1)

# Display the cap
capMapper1 = vtkPolyDataMapper()
capMapper1.SetInputConnection(clipper1.GetOutputPort(1))

capActor1 = vtkActor()
capActor1.SetMapper(capMapper1)
capActor1.GetProperty().SetColor(1,0,0)

# Clipper - appended spheres with cell data
clipper2 = vtkPolyDataPlaneClipper()
clipper2.SetInputConnection(pd2cd.GetOutputPort())
clipper2.SetPlane(plane)
clipper2.SetBatchSize(10000)
clipper2.ClippingLoopsOff()
clipper2.CappingOn()
clipper2.Update()

# Display the clipped cells
mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection(clipper2.GetOutputPort())
mapper2.SetScalarModeToUseCellFieldData()
mapper2.SelectColorArray("Elevation")
mapper2.SetScalarRange(clipper2.GetOutput().GetCellData().GetArray("Elevation").GetRange())

actor2 = vtkActor()
actor2.SetMapper(mapper2)

# Display the cap
capMapper2 = vtkPolyDataMapper()
capMapper2.SetInputConnection(clipper2.GetOutputPort(1))

capActor2 = vtkActor()
capActor2.SetMapper(capMapper2)
capActor2.GetProperty().SetColor(0,1,0)

# Add the actors to the renderer, set the background and size
ren0.AddActor(actor1)
ren0.AddActor(capActor1)
ren0.AddActor(actor2)
ren0.AddActor(capActor2)

ren0.SetBackground(0,0,0)
renWin.SetSize(300,300)
camera = ren0.GetActiveCamera()
camera.SetPosition(1,1,1)
ren0.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
