#!/usr/bin/env python

from vtkmodules.vtkCommonMath import vtkRungeKutta4
from vtkmodules.vtkFiltersCore import vtkStructuredGridOutlineFilter
from vtkmodules.vtkFiltersFlowPaths import vtkStreamTracer
from vtkmodules.vtkFiltersModeling import vtkRuledSurfaceFilter
from vtkmodules.vtkFiltersSources import vtkLineSource
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
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

# create planes
# Create the RenderWindow, Renderer
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName( vtkGetDataRoot() + '/Data/combxyz.bin' )
pl3d.SetQFileName( vtkGetDataRoot() + '/Data/combq.bin' )
pl3d.SetScalarFunctionNumber( 100 )
pl3d.SetVectorFunctionNumber( 202 )
pl3d.Update()
pl3d_output = pl3d.GetOutput().GetBlock(0)

outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

seeds = vtkLineSource()
seeds.SetPoint1(15, -5, 32)
seeds.SetPoint2(15, 5, 32)
seeds.SetResolution(10)

integ = vtkRungeKutta4()

sl = vtkStreamTracer()
sl.SetIntegrator(integ)
sl.SetInputData(pl3d_output)
sl.SetSourceConnection(seeds.GetOutputPort())
sl.SetMaximumPropagation(100)
sl.SetInitialIntegrationStep(0.1)
sl.SetIntegrationDirectionToBackward()

scalarSurface = vtkRuledSurfaceFilter ()
scalarSurface.SetInputConnection(sl.GetOutputPort())
scalarSurface.SetOffset(0)
scalarSurface.SetOnRatio(2)
scalarSurface.PassLinesOn()
scalarSurface.SetRuledModeToResample()
scalarSurface.SetResolution(100,1)
scalarSurface.SetDistanceFactor(30)

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(scalarSurface.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)

mmapper = vtkPolyDataMapper()
mmapper.SetInputConnection(seeds.GetOutputPort())
mactor = vtkActor()
mactor.SetMapper(mmapper)
ren.AddActor(mactor)

ren.AddActor(actor)
ren.AddActor(outlineActor)

cam=ren.GetActiveCamera()
cam.SetClippingRange( 3.95297, 50 )
cam.SetFocalPoint( 8.88908, 0.595038, 29.3342 )
cam.SetPosition( -12.3332, 31.7479, 41.2387 )
cam.SetViewUp( 0.060772, -0.319905, 0.945498 )

renWin.Render()
iren.Start()
