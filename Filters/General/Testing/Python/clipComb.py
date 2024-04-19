#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkImplicitBoolean,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import vtkStructuredGridOutlineFilter
from vtkmodules.vtkFiltersGeneral import vtkClipDataSet
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
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
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

output = pl3d.GetOutput().GetBlock(0)

# create a crazy implicit function
center = output.GetCenter()
sphere = vtkSphere()
sphere.SetCenter(center)
sphere.SetRadius(2.0)

sphere2 = vtkSphere()
sphere2.SetCenter(center[0] + 4.0, center[1], center[2])
sphere2.SetRadius(4.0)

boolOp = vtkImplicitBoolean()
boolOp.SetOperationTypeToUnion()
boolOp.AddFunction(sphere)
boolOp.AddFunction(sphere2)

# clip the structured grid to produce a tetrahedral mesh
clip = vtkClipDataSet()
clip.SetInputData(output)
clip.SetClipFunction(boolOp)
clip.InsideOutOn()

gf = vtkGeometryFilter()
gf.SetInputConnection(clip.GetOutputPort())

clipMapper = vtkPolyDataMapper()
clipMapper.SetInputConnection(gf.GetOutputPort())

clipActor = vtkActor()
clipActor.SetMapper(clipMapper)

outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(clipActor)
ren1.AddActor(outlineActor)

ren1.SetBackground(1, 1, 1)
renWin.SetSize(250, 250)
ren1.SetBackground(0.1, 0.2, 0.4)

cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(8.88908, 0.595038, 29.3342)
cam1.SetPosition(-12.3332, 31.7479, 41.2387)
cam1.SetViewUp(0.060772, -0.319905, 0.945498)
iren.Initialize()

# render the image
#
renWin.Render()

#iren.Start()
