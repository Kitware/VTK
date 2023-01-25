#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkIdList,
    vtkPoints,
)
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkCommonDataModel import vtkUnstructuredGrid
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import (
    vtkExtractEdges,
    vtkGlyph3D,
    vtkThresholdPoints,
    vtkTubeFilter,
)
from vtkmodules.vtkFiltersGeneral import (
    vtkClipDataSet,
    vtkShrinkFilter,
    vtkTransformPolyDataFilter,
)
from vtkmodules.vtkFiltersSources import (
    vtkCubeSource,
    vtkSphereSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingFreeType import vtkVectorText
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# define a Single Cube
Scalars = vtkFloatArray()
Scalars.InsertNextValue(1.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(1.0)

Points = vtkPoints()
Points.InsertNextPoint(0, 0, 0)
Points.InsertNextPoint(1, 0, 0)
Points.InsertNextPoint(0, 1, 0)
Points.InsertNextPoint(0, 0, 1)

Ids = vtkIdList()
Ids.InsertNextId(0)
Ids.InsertNextId(1)
Ids.InsertNextId(2)
Ids.InsertNextId(3)

grid = vtkUnstructuredGrid()
grid.Allocate(10, 10)
grid.InsertNextCell(10, Ids)
grid.SetPoints(Points)
grid.GetPointData().SetScalars(Scalars)

# Clip the tetra
clipper = vtkClipDataSet()
clipper.SetInputData(grid)
clipper.SetValue(0.5)
clipper.Update()

# build tubes for the triangle edges
#
tetEdges = vtkExtractEdges()
tetEdges.SetInputConnection(clipper.GetOutputPort())

tetEdgeTubes = vtkTubeFilter()
tetEdgeTubes.SetInputConnection(tetEdges.GetOutputPort())
tetEdgeTubes.SetRadius(.005)
tetEdgeTubes.SetNumberOfSides(6)

tetEdgeMapper = vtkPolyDataMapper()
tetEdgeMapper.SetInputConnection(tetEdgeTubes.GetOutputPort())
tetEdgeMapper.ScalarVisibilityOff()

tetEdgeActor = vtkActor()
tetEdgeActor.SetMapper(tetEdgeMapper)
tetEdgeActor.GetProperty().SetDiffuseColor(GetRGBColor('lamp_black'))
tetEdgeActor.GetProperty().SetSpecular(.4)
tetEdgeActor.GetProperty().SetSpecularPower(10)

# shrink the triangles so we can see each one
aShrinker = vtkShrinkFilter()
aShrinker.SetShrinkFactor(1)
aShrinker.SetInputConnection(clipper.GetOutputPort())

aMapper = vtkDataSetMapper()
aMapper.ScalarVisibilityOff()
aMapper.SetInputConnection(aShrinker.GetOutputPort())

Tets = vtkActor()
Tets.SetMapper(aMapper)
Tets.GetProperty().SetDiffuseColor(GetRGBColor('banana'))

# build a model of the cube
Edges = vtkExtractEdges()
Edges.SetInputData(grid)

Tubes = vtkTubeFilter()
Tubes.SetInputConnection(Edges.GetOutputPort())
Tubes.SetRadius(.01)
Tubes.SetNumberOfSides(6)

TubeMapper = vtkPolyDataMapper()
TubeMapper.SetInputConnection(Tubes.GetOutputPort())
TubeMapper.ScalarVisibilityOff()

CubeEdges = vtkActor()
CubeEdges.SetMapper(TubeMapper)
CubeEdges.GetProperty().SetDiffuseColor(GetRGBColor('khaki'))
CubeEdges.GetProperty().SetSpecular(.4)
CubeEdges.GetProperty().SetSpecularPower(10)

# build the vertices of the cube
#
Sphere = vtkSphereSource()
Sphere.SetRadius(0.04)
Sphere.SetPhiResolution(20)
Sphere.SetThetaResolution(20)

ThresholdIn = vtkThresholdPoints()
ThresholdIn.SetInputData(grid)
ThresholdIn.ThresholdByUpper(.5)

Vertices = vtkGlyph3D()
Vertices.SetInputConnection(ThresholdIn.GetOutputPort())
Vertices.SetSourceConnection(Sphere.GetOutputPort())

SphereMapper = vtkPolyDataMapper()
SphereMapper.SetInputConnection(Vertices.GetOutputPort())
SphereMapper.ScalarVisibilityOff()

CubeVertices = vtkActor()
CubeVertices.SetMapper(SphereMapper)
CubeVertices.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))

# define the text for the labels
caseLabel = vtkVectorText()
caseLabel.SetText("Case 1")

aLabelTransform = vtkTransform()
aLabelTransform.Identity()
aLabelTransform.Translate(-.2, 0, 1.25)
aLabelTransform.Scale(.05, .05, .05)

labelTransform = vtkTransformPolyDataFilter()
labelTransform.SetTransform(aLabelTransform)
labelTransform.SetInputConnection(caseLabel.GetOutputPort())

labelMapper = vtkPolyDataMapper()
labelMapper.SetInputConnection(labelTransform.GetOutputPort())

labelActor = vtkActor()
labelActor.SetMapper(labelMapper)

# define the base
baseModel = vtkCubeSource()
baseModel.SetXLength(1.5)
baseModel.SetYLength(.01)
baseModel.SetZLength(1.5)

baseMapper = vtkPolyDataMapper()
baseMapper.SetInputConnection(baseModel.GetOutputPort())

base = vtkActor()
base.SetMapper(baseMapper)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# position the base
base.SetPosition(.5, -.09, .5)

ren1.AddActor(tetEdgeActor)
ren1.AddActor(base)
ren1.AddActor(labelActor)
ren1.AddActor(CubeEdges)
ren1.AddActor(CubeVertices)
ren1.AddActor(Tets)
ren1.SetBackground(GetRGBColor('slate_grey'))

grid.Modified()

renWin.SetSize(400, 400)

ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.2)
ren1.GetActiveCamera().Azimuth(30)
ren1.GetActiveCamera().Elevation(20)
ren1.ResetCameraClippingRange()

renWin.Render()

iren.Initialize()

def cases (id, mask):
    i = 0
    while i < 4:
        m = mask[i]
        if m & id == 0:
            Scalars.SetValue(i, 0)
        else:
            Scalars.SetValue(i, 1)
        caseLabel.SetText("Case " + str(id))
        i += 1

    grid.Modified()
    renWin.Render()

mask = [1, 2, 4, 8, 16, 32]

cases(3, mask)

# iren.Start()
