#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkFiltersCore import vtkGlyph2D
from vtkmodules.vtkFiltersSources import vtkGlyphSource2D
from vtkmodules.vtkRenderingCore import (
    vtkActor2D,
    vtkPolyDataMapper2D,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create some random points, scalars, and vectors to glyph
#
pd = vtkPolyData()

pts = vtkPoints()

scalars = vtkFloatArray()

vectors = vtkFloatArray()
vectors.SetNumberOfComponents(3)

pd.SetPoints(pts)
pd.GetPointData().SetScalars(scalars)
pd.GetPointData().SetVectors(vectors)

math = vtkMath()
size = 500
i = 0
while i < 100:
    pts.InsertNextPoint(math.Random(0, size - 1), math.Random(0, size - 1), 0.0)
    scalars.InsertNextValue(math.Random(0.0, 5))
    vectors.InsertNextTuple3(math.Random(-1, 1), math.Random(-1, 1), 0.0)
    i += 1

gs = vtkGlyphSource2D()
gs.SetGlyphTypeToCircle()
gs.SetScale(20)
gs.FilledOff()
gs.CrossOn()
gs.Update()

gs1 = vtkGlyphSource2D()
gs1.SetGlyphTypeToTriangle()
gs1.SetScale(20)
gs1.FilledOff()
gs1.CrossOn()
gs1.Update()

gs2 = vtkGlyphSource2D()
gs2.SetGlyphTypeToSquare()
gs2.SetScale(20)
gs2.FilledOff()
gs2.CrossOn()
gs2.Update()

gs3 = vtkGlyphSource2D()
gs3.SetGlyphTypeToDiamond()
gs3.SetScale(20)
gs3.FilledOff()
gs3.CrossOn()
gs3.Update()

gs4 = vtkGlyphSource2D()
gs4.SetGlyphTypeToDiamond()
gs4.SetScale(20)
gs4.FilledOn()
gs4.DashOn()
gs4.CrossOff()
gs4.Update()

gs5 = vtkGlyphSource2D()
gs5.SetGlyphTypeToThickArrow()
gs5.SetScale(20)
gs5.FilledOn()
gs5.CrossOff()
gs5.Update()

# Create a table of glyphs
glypher = vtkGlyph2D()
glypher.SetInputData(pd)
glypher.SetSourceData(0, gs.GetOutput())
glypher.SetSourceData(1, gs1.GetOutput())
glypher.SetSourceData(2, gs2.GetOutput())
glypher.SetSourceData(3, gs3.GetOutput())
glypher.SetSourceData(4, gs4.GetOutput())
glypher.SetSourceData(5, gs5.GetOutput())
glypher.SetIndexModeToScalar()
glypher.SetRange(0, 5)
glypher.SetScaleModeToDataScalingOff()

mapper = vtkPolyDataMapper2D()
mapper.SetInputConnection(glypher.GetOutputPort())
mapper.SetScalarRange(0, 5)

glyphActor = vtkActor2D()
glyphActor.SetMapper(mapper)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(glyphActor)
ren1.SetBackground(1, 1, 1)
renWin.SetSize(size, size)
renWin.Render()

# render the image
#
iren.Initialize()
#iren.Start()
