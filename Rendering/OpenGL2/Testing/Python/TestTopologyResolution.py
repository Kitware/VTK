#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkMultiBlockDataSet
from vtkmodules.vtkCommonExecutionModel import vtkTrivialProducer
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkFiltersSources import vtkCubeSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCompositePolyDataMapper,
    vtkLight,
    vtkPolyDataMapper,
    vtkProperty,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script shows a cube with edges and faces (using a coincident
# topology offset for polygons) and with ambient, diffuse and specular
# lighting.
# Three polydata mappers are tested with (top row) and without
# (bottom row) lighting:
# 1) left renderer shows a vtkPolyDataMapper which effectively is a
#    vtkOpenGLPolyDataMapper
# 2) right renderer shows a vtkCompositePolyDataMapper which delegates
#    to a vtkOpenGLPolyDataMapper

# The cube
cube = vtkCubeSource()
rib=float(1.)
cube.SetBounds(-rib/2,rib/2,-rib/2,rib/2,-rib/2,rib/2)
cube.SetCenter(0,0,0)
cube.Update()

# Rotate the cube to show the faces being displaced
transform = vtkTransform()
transform.Identity()
transform.RotateX(45)
transform.RotateY(45)
transformer=vtkTransformPolyDataFilter()
transformer.SetInputConnection(cube.GetOutputPort())
transformer.SetTransform(transform)
transformer.Update()

# A trivial multi-block to be used as input for
# vtkCompositePolyDataMapper and vtkCompositePolyDataMapper2
mbd = vtkMultiBlockDataSet()
mbd.SetNumberOfBlocks(1)
mbd.SetBlock(0,transformer.GetOutput())
source=vtkTrivialProducer()
source.SetOutput(mbd)

# Set up the render window and the interactor.
renWin=vtkRenderWindow()
renWin.SetSize(600,400)
iren=vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

mappers=list()
actors=list()
renderers=list()

polyMapper = vtkPolyDataMapper()
polyMapper.SetInputConnection(transformer.GetOutputPort())
mappers.append(polyMapper)

mbdMapper = vtkCompositePolyDataMapper()
mbdMapper.SetInputConnection(source.GetOutputPort())
mappers.append(mbdMapper)

x=0
dx=float(1)/len(mappers)

# Define the property: lighting intensities, color,
# edge visibility, point visibility
p1=vtkProperty()
p1.SetColor(1,0,0)
p1.LightingOff()
p1.SetAmbient(0.2)
p1.SetDiffuse(0.7)
p1.SetSpecular(0.4)
p1.SetSpecularPower(35)
p1.EdgeVisibilityOn()
p1.SetEdgeColor(1,1,1)
p1.VertexVisibilityOn()
p1.SetVertexColor(0,1,0)
p1.SetPointSize(4)

# Top row has lighting on
p2=vtkProperty()
p2.DeepCopy(p1)
p2.LightingOn()

light=vtkLight()
light.SetPosition(1,1,1)

for m in mappers:
    # Push back the polygons
    m.SetRelativeCoincidentTopologyPolygonOffsetParameters(0,2)

    # Bottom renderer shows cube without lighting
    actors.append(vtkActor())
    a1=actors[-1]
    a1.SetMapper(m)
    a1.SetProperty(p1)
    renderers.append(vtkRenderer())
    r1 = renderers[-1]
    r1.AddActor(a1)
    r1.RemoveAllLights()
    r1.AddLight(light)
    r1.SetViewport(x,0,x+dx,0.5)
    renWin.AddRenderer(r1)

    # Top renderer shows cube with lighting
    actors.append(vtkActor())
    a2=actors[-1]
    a2.SetMapper(m)
    a2.SetProperty(p2)
    renderers.append(vtkRenderer())
    r2 = renderers[-1]
    r2.AddActor(a2)
    r2.RemoveAllLights()
    r2.AddLight(light)
    r2.SetViewport(x,0.5,x+dx,1.0)
    renWin.AddRenderer(r2)
    x=x+dx

iren.Initialize()
renWin.Render()
# --- end of script ---
