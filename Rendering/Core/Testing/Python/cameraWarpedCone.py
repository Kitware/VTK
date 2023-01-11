#!/usr/bin/env python

from vtkmodules.vtkCommonMath import vtkMatrix4x4
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderer,
)
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

# create a rendering window and renderer
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(300,300)

# create an actor and give it cone geometry
cone = vtkConeSource()
cone.SetResolution(8)
coneMapper = vtkPolyDataMapper()
coneMapper.SetInputConnection(cone.GetOutputPort())
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)

# create a transform and distort the camera using it
mat = vtkMatrix4x4()
mat.SetElement(0,0,0.5)
mat.SetElement(0,1,0)
mat.SetElement(0,2,0)
mat.SetElement(0,3,0)
mat.SetElement(1,0,0)
mat.SetElement(1,1,1)
mat.SetElement(1,2,0)
mat.SetElement(1,3,0)
mat.SetElement(2,0,0)
mat.SetElement(2,1,0)
mat.SetElement(2,2,1)
mat.SetElement(2,3,0)
mat.SetElement(3,0,0)
mat.SetElement(3,1,0)
mat.SetElement(3,2,0)
mat.SetElement(3,3,1)

trans = vtkTransform()
trans.SetMatrix(mat)

# assign our actor to the renderer
ren.AddActor(coneActor)

ren.ResetCamera();
ren.GetActiveCamera().SetUserTransform(trans);

renWin.Render()
