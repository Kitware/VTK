#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkDoubleArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersCore import (
    vtkAppendPolyData,
    vtkStaticCleanPolyData,
)
from vtkmodules.vtkIOImage import vtkTIFFReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTexture,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the merging of points with texture data.
# Points should only be merged if they have identical
# geometric positions and data (in this case texture
# coordinates).

# The texture map
cheers = vtkTIFFReader()
cheers.SetFileName(VTK_DATA_ROOT + "/Data/beach.tif")

imageTexture = vtkTexture()
imageTexture.InterpolateOn()
imageTexture.SetInputConnection(cheers.GetOutputPort())

# Create four planes with some coincident points. Some of
# points have identical texture coordinates, some do not.
# First plane patch.
pd0 = vtkPolyData()
p0 = vtkPoints()
polys0 = vtkCellArray()
pd0.SetPoints(p0)
pd0.SetPolys(polys0)

p0.SetNumberOfPoints(9)
p0.SetPoint(0, 0,4,0)
p0.SetPoint(1, 1,4,0)
p0.SetPoint(2, 2,4,0)
p0.SetPoint(3, 0,5,0)
p0.SetPoint(4, 1,5,0)
p0.SetPoint(5, 2,5,0)
p0.SetPoint(6, 0,6,0)
p0.SetPoint(7, 1,6,0)
p0.SetPoint(8, 2,6,0)

t0 = vtkDoubleArray()
t0.SetName("TCoords")
t0.SetNumberOfComponents(2)
t0.SetNumberOfTuples(9)
t0.SetTuple2(0, 0,0)
t0.SetTuple2(1, 0,0.5)
t0.SetTuple2(2, 0,1.0)
t0.SetTuple2(3, 0.5,0)
t0.SetTuple2(4, 0.5,0.5)
t0.SetTuple2(5, 0.5,1.0)
t0.SetTuple2(6, 1.0,0)
t0.SetTuple2(7, 1.0,0.5)
t0.SetTuple2(8, 1.0,1.0)
pd0.GetPointData().SetTCoords(t0)

pts = [0,1,4,3]
polys0.InsertNextCell(4,pts)
pts = [1,2,5,4]
polys0.InsertNextCell(4,pts)
pts = [3,4,7,6]
polys0.InsertNextCell(4,pts)
pts = [4,5,8,7]
polys0.InsertNextCell(4,pts)

# Second plane patch
pd1 = vtkPolyData()
p1 = vtkPoints()
polys1 = vtkCellArray()
pd1.SetPoints(p1)
pd1.SetPolys(polys1)

p1.SetNumberOfPoints(9)
p1.SetPoint(0, 2,4,0)
p1.SetPoint(1, 3,4,0)
p1.SetPoint(2, 4,4,0)
p1.SetPoint(3, 2,5,0)
p1.SetPoint(4, 3,5,0)
p1.SetPoint(5, 4,5,0)
p1.SetPoint(6, 2,6,0)
p1.SetPoint(7, 3,6,0)
p1.SetPoint(8, 4,6,0)

t1 = vtkDoubleArray()
t1.SetName("TCoords")
t1.SetNumberOfComponents(2)
t1.SetNumberOfTuples(9)
t1.SetTuple2(0, 0,0)
t1.SetTuple2(1, 0,0.5)
t1.SetTuple2(2, 0,1.0)
t1.SetTuple2(3, 0.5,0)
t1.SetTuple2(4, 0.5,0.5)
t1.SetTuple2(5, 0.5,1.0)
t1.SetTuple2(6, 1.0,0)
t1.SetTuple2(7, 1.0,0.5)
t1.SetTuple2(8, 1.0,1.0)
pd1.GetPointData().SetTCoords(t1)

pts = [0,1,4,3]
polys1.InsertNextCell(4,pts)
pts = [1,2,5,4]
polys1.InsertNextCell(4,pts)
pts = [3,4,7,6]
polys1.InsertNextCell(4,pts)
pts = [4,5,8,7]
polys1.InsertNextCell(4,pts)

# Third plane patch
pd2 = vtkPolyData()
p2 = vtkPoints()
polys2 = vtkCellArray()
pd2.SetPoints(p2)
pd2.SetPolys(polys2)

p2.SetNumberOfPoints(15)
p2.SetPoint(0, 0,0,0)
p2.SetPoint(1, 1,0,0)
p2.SetPoint(2, 2,0,0)
p2.SetPoint(3, 0,1,0)
p2.SetPoint(4, 1,1,0)
p2.SetPoint(5, 2,1,0)
p2.SetPoint(6, 0,2,0)
p2.SetPoint(7, 1,2,0)
p2.SetPoint(8, 2,2,0)
p2.SetPoint(9, 0,3,0)
p2.SetPoint(10, 1,3,0)
p2.SetPoint(11, 2,3,0)
p2.SetPoint(12, 0,4,0)
p2.SetPoint(13, 1,4,0)
p2.SetPoint(14, 2,4,0)

t2 = vtkDoubleArray()
t2.SetName("TCoords")
t2.SetNumberOfComponents(2)
t2.SetNumberOfTuples(15)
t2.SetTuple2(0, 0.00,0.0)
t2.SetTuple2(1, 0.25,0.0)
t2.SetTuple2(2, 0.50,0.0)
t2.SetTuple2(3, 0.00,0.25)
t2.SetTuple2(4, 0.25,0.25)
t2.SetTuple2(5, 0.50,0.25)
t2.SetTuple2(6, 0.00,0.5)
t2.SetTuple2(7, 0.25,0.5)
t2.SetTuple2(8, 0.50,0.5)
t2.SetTuple2(9, 0.00,0.75)
t2.SetTuple2(10, 0.25,0.75)
t2.SetTuple2(11, 0.50,0.75)
t2.SetTuple2(12, 0.00,1.0)
t2.SetTuple2(13, 0.25,1.0)
t2.SetTuple2(14, 0.50,1.0)
pd2.GetPointData().SetTCoords(t2)

pts = [0,1,4,3]
polys2.InsertNextCell(4,pts)
pts = [1,2,5,4]
polys2.InsertNextCell(4,pts)
pts = [3,4,7,6]
polys2.InsertNextCell(4,pts)
pts = [4,5,8,7]
polys2.InsertNextCell(4,pts)
pts = [6,7,10,9]
polys2.InsertNextCell(4,pts)
pts = [7,8,11,10]
polys2.InsertNextCell(4,pts)
pts = [9,10,13,12]
polys2.InsertNextCell(4,pts)
pts = [10,11,14,13]
polys2.InsertNextCell(4,pts)

# Fourth plane patch
pd3 = vtkPolyData()
p3 = vtkPoints()
polys3 = vtkCellArray()
pd3.SetPoints(p3)
pd3.SetPolys(polys3)

p3.SetNumberOfPoints(15)
p3.SetPoint(0, 2,0,0)
p3.SetPoint(1, 3,0,0)
p3.SetPoint(2, 4,0,0)
p3.SetPoint(3, 2,1,0)
p3.SetPoint(4, 3,1,0)
p3.SetPoint(5, 4,1,0)
p3.SetPoint(6, 2,2,0)
p3.SetPoint(7, 3,2,0)
p3.SetPoint(8, 4,2,0)
p3.SetPoint(9, 2,3,0)
p3.SetPoint(10, 3,3,0)
p3.SetPoint(11, 4,3,0)
p3.SetPoint(12, 2,4,0)
p3.SetPoint(13, 3,4,0)
p3.SetPoint(14, 4,4,0)

t3 = vtkDoubleArray()
t3.SetName("TCoords")
t3.SetNumberOfComponents(2)
t3.SetNumberOfTuples(15)
t3.SetTuple2(0, 0.50,0.0)
t3.SetTuple2(1, 0.75,0.0)
t3.SetTuple2(2, 1.0,0.0)
t3.SetTuple2(3, 0.50,0.25)
t3.SetTuple2(4, 0.75,0.25)
t3.SetTuple2(5, 1.0,0.25)
t3.SetTuple2(6, 0.50,0.5)
t3.SetTuple2(7, 0.75,0.5)
t3.SetTuple2(8, 1.0,0.5)
t3.SetTuple2(9, 0.50,0.75)
t3.SetTuple2(10, 0.75,0.75)
t3.SetTuple2(11, 1.0,0.75)
t3.SetTuple2(12, 0.50,1.0)
t3.SetTuple2(13, 0.75,1.0)
t3.SetTuple2(14, 1.0,1.0)
pd3.GetPointData().SetTCoords(t3)

pts = [0,1,4,3]
polys3.InsertNextCell(4,pts)
pts = [1,2,5,4]
polys3.InsertNextCell(4,pts)
pts = [3,4,7,6]
polys3.InsertNextCell(4,pts)
pts = [4,5,8,7]
polys3.InsertNextCell(4,pts)
pts = [6,7,10,9]
polys3.InsertNextCell(4,pts)
pts = [7,8,11,10]
polys3.InsertNextCell(4,pts)
pts = [9,10,13,12]
polys3.InsertNextCell(4,pts)
pts = [10,11,14,13]
polys3.InsertNextCell(4,pts)

# Put them all together
append = vtkAppendPolyData()
append.AddInputData(pd0)
append.AddInputData(pd1)
append.AddInputData(pd2)
append.AddInputData(pd3)
append.Update()

numAppendPts = append.GetOutput().GetNumberOfPoints()
print("Number of points before merging: ", append.GetOutput().GetNumberOfPoints())
assert(numAppendPts == 48)

merge = vtkStaticCleanPolyData()
merge.SetInputConnection(append.GetOutputPort())
merge.SetMergingArray("TCoords")
merge.Update()

numMergedPts = merge.GetOutput().GetNumberOfPoints()
print("Number of points after merging: ", numMergedPts)
assert(numMergedPts == 43)

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(merge.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)
actor.SetTexture(imageTexture)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(256,256)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(0,0,0)
ren1.GetActiveCamera().SetPosition(0,0,1)
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.ResetCamera()

iren.Initialize()
iren.Start()
# --- end of script --
