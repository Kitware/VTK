#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create some random points, scalars, and vectors to glyph
#
pd = vtk.vtkPolyData()

pts = vtk.vtkPoints()

scalars = vtk.vtkFloatArray()

vectors = vtk.vtkFloatArray()
vectors.SetNumberOfComponents(3)

pd.SetPoints(pts)
pd.GetPointData().SetScalars(scalars)
pd.GetPointData().SetVectors(vectors)

math = vtk.vtkMath()
size = 500
i = 0
while i < 100:
    pts.InsertNextPoint(math.Random(0, size - 1), math.Random(0, size - 1), 0.0)
    scalars.InsertNextValue(math.Random(0.0, 5))
    vectors.InsertNextTuple3(math.Random(-1, 1), math.Random(-1, 1), 0.0)
    i += 1

gs = vtk.vtkGlyphSource2D()
gs.SetGlyphTypeToCircle()
gs.SetScale(20)
gs.FilledOff()
gs.CrossOn()
gs.Update()

gs1 = vtk.vtkGlyphSource2D()
gs1.SetGlyphTypeToTriangle()
gs1.SetScale(20)
gs1.FilledOff()
gs1.CrossOn()
gs1.Update()

gs2 = vtk.vtkGlyphSource2D()
gs2.SetGlyphTypeToSquare()
gs2.SetScale(20)
gs2.FilledOff()
gs2.CrossOn()
gs2.Update()

gs3 = vtk.vtkGlyphSource2D()
gs3.SetGlyphTypeToDiamond()
gs3.SetScale(20)
gs3.FilledOff()
gs3.CrossOn()
gs3.Update()

gs4 = vtk.vtkGlyphSource2D()
gs4.SetGlyphTypeToDiamond()
gs4.SetScale(20)
gs4.FilledOn()
gs4.DashOn()
gs4.CrossOff()
gs4.Update()

gs5 = vtk.vtkGlyphSource2D()
gs5.SetGlyphTypeToThickArrow()
gs5.SetScale(20)
gs5.FilledOn()
gs5.CrossOff()
gs5.Update()

# Create a table of glyphs
glypher = vtk.vtkGlyph2D()
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

mapper = vtk.vtkPolyDataMapper2D()
mapper.SetInputConnection(glypher.GetOutputPort())
mapper.SetScalarRange(0, 5)

glyphActor = vtk.vtkActor2D()
glyphActor.SetMapper(mapper)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
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
