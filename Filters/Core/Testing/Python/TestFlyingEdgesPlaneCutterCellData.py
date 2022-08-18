#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control size of test
res = 10

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtk.vtkRenderer()
ren0.SetViewport(0,0,0.5,1)
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: sample a sphere across a volume
sphere = vtk.vtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

sample = vtk.vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.ComputeNormalsOff()
sample.Update()

# Create some cell data
pd2cd = vtk.vtkPointDataToCellData()
pd2cd.SetInputConnection(sample.GetOutputPort())
pd2cd.PassPointDataOn()
pd2cd.Update()

# The cut plane
plane = vtk.vtkPlane()
plane.SetOrigin(-.2,-.2,-.2)
plane.SetNormal(1,1,1)

# Perform the cutting on named scalars
cut = vtk.vtkFlyingEdgesPlaneCutter()
cut.SetInputConnection(pd2cd.GetOutputPort())
cut.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_POINTS, "scalars")
cut.SetPlane(plane)
cut.ComputeNormalsOff()
cut.InterpolateAttributesOn()

# Time the execution of the filter
timer = vtk.vtkExecutionTimer()
timer.SetFilter(cut)
cut.Update()
CG = timer.GetElapsedWallClockTime()
print ("Cut and interpolate volume:", CG)

# Display the point data
cutMapper = vtk.vtkPolyDataMapper()
cutMapper.SetInputConnection(cut.GetOutputPort())

cutActor = vtk.vtkActor()
cutActor.SetMapper(cutMapper)

# Display the cell data
cellCutMapper = vtk.vtkPolyDataMapper()
cellCutMapper.SetInputConnection(cut.GetOutputPort())
cellCutMapper.SetScalarModeToUseCellData()

cellCutActor = vtk.vtkActor()
cellCutActor.SetMapper(cellCutMapper)

# Outline
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(outlineActor)
ren0.AddActor(cutActor)
ren0.SetBackground(0,0,0)
ren1.AddActor(outlineActor)
ren1.AddActor(cellCutActor)
ren1.SetBackground(0,0,0)

renWin.SetSize(600,300)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
