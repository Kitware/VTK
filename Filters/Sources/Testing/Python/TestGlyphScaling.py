#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


GLYPH_TYPES = [
  'VTK_NO_GLYPH',
  'VTK_VERTEX_GLYPH',
  'VTK_DASH_GLYPH',
  'VTK_CROSS_GLYPH',
  'VTK_THICKCROSS_GLYPH',
  'VTK_TRIANGLE_GLYPH',
  'VTK_SQUARE_GLYPH',
  'VTK_CIRCLE_GLYPH',
  'VTK_DIAMOND_GLYPH',
  'VTK_ARROW_GLYPH',
  'VTK_THICKARROW_GLYPH',
  'VTK_HOOKEDARROW_GLYPH',
  'VTK_EDGEARROW_GLYPH'
]

numRows = len(GLYPH_TYPES)
actors = []
fixedScales = [ 0.5, 1.0, 1.5, 2.0 ]

for i in range(numRows):
  for j in range(1, 5):
    points = vtk.vtkPoints()
    points.InsertNextPoint(j*3, i*3, 0)

    polydata = vtk.vtkPolyData()
    polydata.SetPoints(points)

    glyphSource = vtk.vtkGlyphSource2D()

    glyphSource.SetGlyphType(i)
    glyphSource.FilledOff()
    glyphSource.SetResolution(25)
    glyphSource.SetScale(fixedScales[j-1])

    if GLYPH_TYPES[i] == 'VTK_TRIANGLE_GLYPH':
      glyphSource.SetRotationAngle(90)

    glyph2D = vtk.vtkGlyph2D()
    glyph2D.SetSourceConnection(glyphSource.GetOutputPort())
    glyph2D.SetInputData(polydata)
    glyph2D.Update()

    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(glyph2D.GetOutputPort())
    mapper.Update()

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)

    actors.append(actor)

# Set up the renderer, render window, and interactor.
renderer = vtk.vtkRenderer()

for actor in actors:
  renderer.AddActor(actor)

renWin = vtk.vtkRenderWindow()
renWin.SetSize(400,600)
renWin.AddRenderer(renderer)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
renWin.Render()
