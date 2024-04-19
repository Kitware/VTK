#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersGeneral import vtkContourTriangulator
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleTrackballCamera
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

# The purpose of the test is to show how to use
# vtkContourTriangulator::TriangulateContours method.

# Manually create polygons; some have internal holes.
pts = vtkPoints()
pts.SetNumberOfPoints(12)
pts.SetPoint(0, 0,0,0)
pts.SetPoint(1, 10,0,0)
pts.SetPoint(2, 10,10,0)
pts.SetPoint(3, 0,10,0)
pts.SetPoint(4, 2,2,0)
pts.SetPoint(5, 8,2,0)
pts.SetPoint(6, 8,8,0)
pts.SetPoint(7, 2,8,0)
pts.SetPoint(8, 12,0,0)
pts.SetPoint(9, 22,0,0)
pts.SetPoint(10, 22,10,0)
pts.SetPoint(11, 12,10,0)

# Outer loop
loops = vtkCellArray()
conn = [0,1]
loops.InsertNextCell(2,conn)
conn = [1,2]
loops.InsertNextCell(2,conn)
conn = [2,3]
loops.InsertNextCell(2,conn)
conn = [3,0]
loops.InsertNextCell(2,conn)

# Inner loop - reverse ordering (i.e., flipped normal)
conn = [5,4]
loops.InsertNextCell(2,conn)
conn = [6,5]
loops.InsertNextCell(2,conn)
conn = [7,6]
loops.InsertNextCell(2,conn)
conn = [4,7]
loops.InsertNextCell(2,conn)

# Another loop disjoint from first two
conn = [8,9]
loops.InsertNextCell(2,conn)
conn = [9,10]
loops.InsertNextCell(2,conn)
conn = [10,11]
loops.InsertNextCell(2,conn)
conn = [11,8]
loops.InsertNextCell(2,conn)

pd = vtkPolyData()
pd.SetPoints(pts)
pd.SetLines(loops)

# Now triangulate
normal = [0,0,1]
outputCA = vtkCellArray()
ct = vtkContourTriangulator()
ct.TriangulateContours(pd,0,12,outputCA,normal)

outPD = vtkPolyData()
outPD.SetPoints(pts)
outPD.SetPolys(outputCA)

mapper = vtkPolyDataMapper()
mapper.SetInputData(outPD)

actor = vtkActor()
actor.SetMapper(mapper)

# Graphics objects
ren1 = vtkRenderer()
ren1.SetBackground(0,0,0)
ren1.AddActor(actor)
ren1.ResetCamera()

renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(300,300)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
style = vtkInteractorStyleTrackballCamera()
iren.SetInteractorStyle(style)

renWin.Render()
iren.Start()
# --- end of script --
