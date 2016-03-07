#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

res = 100

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
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

# Now create some new attributes to interpolate
cyl = vtk.vtkCylinder()
cyl.SetRadius(0.1)
cyl.SetAxis(1,1,1)

attr = vtk.vtkSampleImplicitFunctionFilter()
attr.SetInputConnection(sample.GetOutputPort())
attr.SetImplicitFunction(cyl)
attr.ComputeGradientsOn()
attr.Update()

# The cut plane
plane = vtk.vtkPlane()
plane.SetOrigin(-.2,-.2,-.2)
plane.SetNormal(1,1,1)

# Perform the cutting on named scalars
cut = vtk.vtkFlyingEdgesPlaneCutter()
cut.SetInputConnection(attr.GetOutputPort())
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

cutMapper = vtk.vtkPolyDataMapper()
cutMapper.SetInputConnection(cut.GetOutputPort())
cutMapper.SetScalarModeToUsePointFieldData()
cutMapper.SelectColorArray("Implicit scalars")

cutActor = vtk.vtkActor()
cutActor.SetMapper(cutMapper)
cutActor.GetProperty().SetColor(1,1,1)
cutActor.GetProperty().SetOpacity(1)

# Look at the vectors
ranPts = vtk.vtkMaskPoints()
ranPts.SetOnRatio(25)
ranPts.SetInputConnection(cut.GetOutputPort())

hhog = vtk.vtkHedgeHog()
hhog.SetInputConnection(ranPts.GetOutputPort())
hhog.SetVectorModeToUseVector()
hhog.SetScaleFactor(0.05)

hhogMapper = vtk.vtkPolyDataMapper()
hhogMapper.SetInputConnection(hhog.GetOutputPort())

hhogActor = vtk.vtkActor()
hhogActor.SetMapper(hhogMapper)
hhogActor.GetProperty().SetColor(1,1,1)
hhogActor.GetProperty().SetOpacity(1)

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
ren1.AddActor(outlineActor)
ren1.AddActor(cutActor)
ren1.AddActor(hhogActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(400,400)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
# --- end of script --
