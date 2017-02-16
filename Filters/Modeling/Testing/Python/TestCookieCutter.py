#!/usr/bin/env python
import vtk

# create planes
# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline. Use a plane source to create an array
# of points; then glyph them. Then cookie cut the glphs.
#
plane = vtk.vtkPlaneSource()
plane.SetXResolution(25)
plane.SetYResolution(25)

# Custom glyph
glyphData = vtk.vtkPolyData()
glyphPts = vtk.vtkPoints()
glyphVerts = vtk.vtkCellArray()
glyphLines = vtk.vtkCellArray()
glyphPolys = vtk.vtkCellArray()
glyphData.SetPoints(glyphPts)
glyphData.SetVerts(glyphVerts)
glyphData.SetLines(glyphLines)
glyphData.SetPolys(glyphPolys)

glyphPts.InsertPoint(0, -0.5,-0.5,0.0)
glyphPts.InsertPoint(1,  0.5,-0.5,0.0)
glyphPts.InsertPoint(2,  0.5, 0.5,0.0)
glyphPts.InsertPoint(3, -0.5, 0.5,0.0)
glyphPts.InsertPoint(4,  0.0,-0.5,0.0)
glyphPts.InsertPoint(5,  0.0,-0.75,0.0)
glyphPts.InsertPoint(6,  0.5, 0.0,0.0)
glyphPts.InsertPoint(7,  0.75, 0.0,0.0)
glyphPts.InsertPoint(8,  0.0, 0.5,0.0)
glyphPts.InsertPoint(9,  0.0, 0.75,0.0)
glyphPts.InsertPoint(10, -0.5,0.0,0.0)
glyphPts.InsertPoint(11, -0.75,0.0,0.0)
glyphPts.InsertPoint(12, 0.0,0.0,0.0)
glyphPts.InsertPoint(13, -0.9,0.0,0.0)

glyphVerts.InsertNextCell(1)
glyphVerts.InsertCellPoint(12)

glyphLines.InsertNextCell(2)
glyphLines.InsertCellPoint(4)
glyphLines.InsertCellPoint(5)
glyphLines.InsertNextCell(2)
glyphLines.InsertCellPoint(6)
glyphLines.InsertCellPoint(7)
glyphLines.InsertNextCell(2)
glyphLines.InsertCellPoint(8)
glyphLines.InsertCellPoint(9)
# The last line is a polyline
glyphLines.InsertNextCell(3)
glyphLines.InsertCellPoint(10)
glyphLines.InsertCellPoint(11)
glyphLines.InsertCellPoint(13)

glyphPolys.InsertNextCell(4)
glyphPolys.InsertCellPoint(0)
glyphPolys.InsertCellPoint(1)
glyphPolys.InsertCellPoint(2)
glyphPolys.InsertCellPoint(3)

glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(plane.GetOutputPort())
glyph.SetSourceData(glyphData)
glyph.SetScaleFactor( 0.02 )

# Create multiple loops for cookie cutting
loops = vtk.vtkPolyData()
loopPts = vtk.vtkPoints()
loopPolys = vtk.vtkCellArray()
loops.SetPoints(loopPts)
loops.SetPolys(loopPolys)

loopPts.SetNumberOfPoints(16)
loopPts.SetPoint(0, -0.35,0.0,0.0)
loopPts.SetPoint(1, 0,-0.35,0.0)
loopPts.SetPoint(2, 0.35,0.0,0.0)
loopPts.SetPoint(3, 0.0,0.35,0.0)

loopPts.SetPoint(4, -0.35,-0.10,0.0)
loopPts.SetPoint(5, -0.35,-0.35,0.0)
loopPts.SetPoint(6, -0.10,-0.35,0.0)

loopPts.SetPoint(7, 0.35,-0.10,0.0)
loopPts.SetPoint(9, 0.35,-0.35,0.0)
loopPts.SetPoint(8, 0.10,-0.35,0.0)

loopPts.SetPoint(10, 0.35,0.10,0.0)
loopPts.SetPoint(11, 0.35,0.35,0.0)
loopPts.SetPoint(12, 0.10,0.35,0.0)

loopPts.SetPoint(13, -0.35,0.10,0.0)
loopPts.SetPoint(14, -0.35,0.35,0.0)
loopPts.SetPoint(15, -0.10,0.35,0.0)

loopPolys.InsertNextCell(4)
loopPolys.InsertCellPoint(0)
loopPolys.InsertCellPoint(1)
loopPolys.InsertCellPoint(2)
loopPolys.InsertCellPoint(3)

loopPolys.InsertNextCell(3)
loopPolys.InsertCellPoint(4)
loopPolys.InsertCellPoint(5)
loopPolys.InsertCellPoint(6)

loopPolys.InsertNextCell(3)
loopPolys.InsertCellPoint(7)
loopPolys.InsertCellPoint(8)
loopPolys.InsertCellPoint(9)

loopPolys.InsertNextCell(3)
loopPolys.InsertCellPoint(10)
loopPolys.InsertCellPoint(11)
loopPolys.InsertCellPoint(12)

loopPolys.InsertNextCell(3)
loopPolys.InsertCellPoint(13)
loopPolys.InsertCellPoint(14)
loopPolys.InsertCellPoint(15)

cookie = vtk.vtkCookieCutter()
cookie.SetInputConnection(glyph.GetOutputPort())
cookie.SetLoopsData(loops)

cookie.Update()
numCells = glyph.GetOutput().GetNumberOfCells()
glyphScalars = vtk.vtkUnsignedCharArray()
glyphScalars.SetNumberOfComponents( 4 )
glyphScalars.SetNumberOfTuples( numCells )
for i in range(numCells):
    glyphScalars.SetTuple4(i, 127, 207, 80, 127)
cookie.GetOutput().GetCellData().SetScalars(glyphScalars)

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(cookie.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

loopMapper = vtk.vtkPolyDataMapper()
loopMapper.SetInputData(loops)

loopActor = vtk.vtkActor()
loopActor.SetMapper(loopMapper)
loopActor.GetProperty().SetColor(1,0,0)
loopActor.GetProperty().SetRepresentationToWireframe()

ren.AddActor(actor)
ren.AddActor(loopActor)

renWin.Render()
#iren.Start()
