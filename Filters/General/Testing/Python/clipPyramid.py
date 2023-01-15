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
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)
Scalars.InsertNextValue(0.0)

Points = vtkPoints()
Points.InsertNextPoint(0, 0, 0)
Points.InsertNextPoint(1, 0, 0)
Points.InsertNextPoint(1, 1, 0)
Points.InsertNextPoint(0, 1, 0)
Points.InsertNextPoint(.5, .5, 1)

Ids = vtkIdList()
Ids.InsertNextId(0)
Ids.InsertNextId(1)
Ids.InsertNextId(2)
Ids.InsertNextId(3)
Ids.InsertNextId(4)

Grid = vtkUnstructuredGrid()
Grid.Allocate(10, 10)
Grid.InsertNextCell(14, Ids)
Grid.SetPoints(Points)
Grid.GetPointData().SetScalars(Scalars)

# Clip the pyramid
clipper = vtkClipDataSet()
clipper.SetInputData(Grid)
clipper.SetValue(0.5)

# build tubes for the triangle edges
#
pyrEdges = vtkExtractEdges()
pyrEdges.SetInputConnection(clipper.GetOutputPort())

pyrEdgeTubes = vtkTubeFilter()
pyrEdgeTubes.SetInputConnection(pyrEdges.GetOutputPort())
pyrEdgeTubes.SetRadius(.005)
pyrEdgeTubes.SetNumberOfSides(6)

pyrEdgeMapper = vtkPolyDataMapper()
pyrEdgeMapper.SetInputConnection(pyrEdgeTubes.GetOutputPort())
pyrEdgeMapper.ScalarVisibilityOff()

pyrEdgeActor = vtkActor()
pyrEdgeActor.SetMapper(pyrEdgeMapper)
pyrEdgeActor.GetProperty().SetDiffuseColor(GetRGBColor('lamp_black'))
pyrEdgeActor.GetProperty().SetSpecular(.4)
pyrEdgeActor.GetProperty().SetSpecularPower(10)

# shrink the triangles so we can see each one
aShrinker = vtkShrinkFilter()
aShrinker.SetShrinkFactor(1)
aShrinker.SetInputConnection(clipper.GetOutputPort())

aMapper = vtkDataSetMapper()
aMapper.ScalarVisibilityOff()
aMapper.SetInputConnection(aShrinker.GetOutputPort())

Pyrs = vtkActor()
Pyrs.SetMapper(aMapper)
Pyrs.GetProperty().SetDiffuseColor(GetRGBColor('banana'))

# build a model of the pyramid
Edges = vtkExtractEdges()
Edges.SetInputData(Grid)

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

# build the vertices of the pyramid
#
Sphere = vtkSphereSource()
Sphere.SetRadius(0.04)
Sphere.SetPhiResolution(20)
Sphere.SetThetaResolution(20)

ThresholdIn = vtkThresholdPoints()
ThresholdIn.SetInputData(Grid)
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

ren1.AddActor(pyrEdgeActor)
ren1.AddActor(base)
ren1.AddActor(labelActor)
ren1.AddActor(CubeEdges)
ren1.AddActor(CubeVertices)
ren1.AddActor(Pyrs)
ren1.SetBackground(GetRGBColor('slate_grey'))

renWin.SetSize(400, 400)

ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.3)
ren1.GetActiveCamera().Elevation(15)
ren1.ResetCameraClippingRange()

renWin.Render()

iren.Initialize()

def cases (id, mask):
    i = 0
    while i < 5:
        m = mask[i]
        if m & id == 0:
            Scalars.SetValue(i, 0)
        else:
            Scalars.SetValue(i, 1)
        caseLabel.SetText("Case " + str(id))
        i += 1

    Grid.Modified()
    renWin.Render()

mask = [1, 2, 4, 8, 16, 32]

cases(20, mask)

# iren.Start()
