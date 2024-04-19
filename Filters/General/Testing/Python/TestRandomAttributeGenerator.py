#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkFiltersCore import (
    vtkPolyDataNormals,
    vtkTensorGlyph,
)
from vtkmodules.vtkFiltersGeneral import vtkRandomAttributeGenerator
from vtkmodules.vtkFiltersSources import (
    vtkPlaneSource,
    vtkSphereSource,
)
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

# Create the RenderWindow, Renderer and interactive renderer
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)

# make sure to have the same regression image on all platforms.
renWin.SetMultiSamples(0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Force a starting random value
raMath = vtkMath()
raMath.RandomSeed(6)

# Generate random attributes on a plane
#
ps = vtkPlaneSource()
ps.SetXResolution(10)
ps.SetYResolution(10)

ag = vtkRandomAttributeGenerator()
ag.SetInputConnection(ps.GetOutputPort())
ag.GenerateAllDataOn()

ss = vtkSphereSource()
ss.SetPhiResolution(16)
ss.SetThetaResolution(32)

tg = vtkTensorGlyph()
tg.SetInputConnection(ag.GetOutputPort())
tg.SetSourceConnection(ss.GetOutputPort())
tg.SetInputArrayToProcess(1,0,0,0,"RandomPointArray")

tg.SetScaleFactor(0.1)
tg.SetMaxScaleFactor(10)
tg.ClampScalingOn()

n = vtkPolyDataNormals()
n.SetInputConnection(tg.GetOutputPort())

pdm = vtkPolyDataMapper()
pdm.SetInputConnection(n.GetOutputPort())

a = vtkActor()
a.SetMapper(pdm)

pm = vtkPolyDataMapper()
pm.SetInputConnection(ps.GetOutputPort())

pa = vtkActor()
pa.SetMapper(pm)

ren1.AddActor(a)
ren1.AddActor(pa)
ren1.SetBackground(0, 0, 0)

renWin.SetSize(300, 300)

renWin.Render()

#iren.Start()
