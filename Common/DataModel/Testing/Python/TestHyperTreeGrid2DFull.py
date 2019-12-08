#!/usr/bin/env python
"""
Create a HTG
without mask
during HTG build we build scalar, used Global Index implicit with SetGlobalIndexStart
SetGlobalIndexStart, one call by HT
"""
import vtk

htg = vtk.vtkHyperTreeGrid()
htg.Initialize()

scalarArray = vtk.vtkDoubleArray()
scalarArray.SetName('scalar')
scalarArray.SetNumberOfValues(0)
htg.GetPointData().AddArray(scalarArray)
htg.GetPointData().SetActiveScalars('scalar')

vecArray = vtk.vtkDoubleArray()
vecArray.SetName('vector')
vecArray.SetNumberOfValues(0)
vecArray.SetNumberOfComponents(3)
htg.GetPointData().AddArray(vecArray)
htg.GetPointData().SetActiveScalars('vector')

htg.SetDimensions([4, 3, 1])
htg.SetBranchFactor(2)

# Rectilinear grid coordinates
xValues = vtk.vtkDoubleArray()
xValues.SetNumberOfValues(4)
xValues.SetValue(0, -1)
xValues.SetValue(1, 0)
xValues.SetValue(2, 1)
xValues.SetValue(3, 2)
htg.SetXCoordinates(xValues)

yValues = vtk.vtkDoubleArray()
yValues.SetNumberOfValues(3)
yValues.SetValue(0, -1)
yValues.SetValue(1, 0)
yValues.SetValue(2, 1)
htg.SetYCoordinates(yValues)

zValues = vtk.vtkDoubleArray()
zValues.SetNumberOfValues(1)
zValues.SetValue(0, 0)
htg.SetZCoordinates(zValues)

# Let's split the various trees
cursor = vtk.vtkHyperTreeGridNonOrientedCursor()
offsetIndex = 0

# ROOT CELL 0
htg.InitializeNonOrientedCursor(cursor, 0, True)
cursor.SetGlobalIndexStart(offsetIndex)

idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 1)
vecArray.InsertComponent(idx, 0, 1)
vecArray.InsertComponent(idx, 1, 1)
vecArray.InsertComponent(idx, 2, 1)

cursor.SubdivideLeaf()

# ROOT CELL 0/[0-3]
cursor.ToChild(0)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 7)
vecArray.InsertComponent(idx, 0, 7)
vecArray.InsertComponent(idx, 1, 7)
vecArray.InsertComponent(idx, 2, 7)
cursor.ToParent()

cursor.ToChild(1)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 8)
vecArray.InsertComponent(idx, 0, 8)
vecArray.InsertComponent(idx, 1, 8)
vecArray.InsertComponent(idx, 2, 8)
cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 9)
vecArray.InsertComponent(idx, 0, 9)
vecArray.InsertComponent(idx, 1, 9)
vecArray.InsertComponent(idx, 2, 9)
cursor.ToParent()

cursor.ToChild(3)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 10)
vecArray.InsertComponent(idx, 0, 10)
vecArray.InsertComponent(idx, 1, 10)
vecArray.InsertComponent(idx, 2, 10)
cursor.ToParent()

# ROOT CELL 0

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 1
htg.InitializeNonOrientedCursor(cursor, 1, True)
cursor.SetGlobalIndexStart(offsetIndex)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 2)
vecArray.InsertComponent(idx, 0, 2)
vecArray.InsertComponent(idx, 1, 2)
vecArray.InsertComponent(idx, 2, 2)

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 2
htg.InitializeNonOrientedCursor(cursor, 2, True)
cursor.SetGlobalIndexStart(offsetIndex)

idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 3)
vecArray.InsertComponent(idx, 0, 3)
vecArray.InsertComponent(idx, 1, 3)
vecArray.InsertComponent(idx, 2, 3)

cursor.SubdivideLeaf()

# ROOT CELL 2/[0-3]
cursor.ToChild(0)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 11)
vecArray.InsertComponent(idx, 0, 11)
vecArray.InsertComponent(idx, 1, 11)
vecArray.InsertComponent(idx, 2, 11)
cursor.ToParent()

cursor.ToChild(1)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 12)
vecArray.InsertComponent(idx, 0, 12)
vecArray.InsertComponent(idx, 1, 12)
vecArray.InsertComponent(idx, 2, 12)
cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 13)
vecArray.InsertComponent(idx, 0, 13)
vecArray.InsertComponent(idx, 1, 13)
vecArray.InsertComponent(idx, 2, 13)
cursor.ToParent()

cursor.ToChild(3)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 14)
vecArray.InsertComponent(idx, 0, 14)
vecArray.InsertComponent(idx, 1, 14)
vecArray.InsertComponent(idx, 2, 14)
cursor.ToParent()

# ROOT CELL 2

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 3
htg.InitializeNonOrientedCursor(cursor, 3, True)
cursor.SetGlobalIndexStart(offsetIndex)

idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 4)
vecArray.InsertComponent(idx, 0, 4)
vecArray.InsertComponent(idx, 1, 4)
vecArray.InsertComponent(idx, 2, 4)

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 4
htg.InitializeNonOrientedCursor(cursor, 4, True)
cursor.SetGlobalIndexStart(offsetIndex)

idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 5)
vecArray.InsertComponent(idx, 0, 5)
vecArray.InsertComponent(idx, 1, 5)
vecArray.InsertComponent(idx, 2, 5)


cursor.SubdivideLeaf()

# ROOT CELL 4/[0-3]
cursor.ToChild(0)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 15)
vecArray.InsertComponent(idx, 0, 15)
vecArray.InsertComponent(idx, 1, 15)
vecArray.InsertComponent(idx, 2, 15)
cursor.ToParent()

cursor.ToChild(1)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 16)
vecArray.InsertComponent(idx, 0, 16)
vecArray.InsertComponent(idx, 1, 16)
vecArray.InsertComponent(idx, 2, 16)
cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 17)
vecArray.InsertComponent(idx, 0, 17)
vecArray.InsertComponent(idx, 1, 17)
vecArray.InsertComponent(idx, 2, 17)
cursor.ToParent()

cursor.ToChild(3)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 18)
vecArray.InsertComponent(idx, 0, 18)
vecArray.InsertComponent(idx, 1, 18)
vecArray.InsertComponent(idx, 2, 18)

cursor.SubdivideLeaf()

# ROOT CELL 4/3/[0-3]
cursor.ToChild(0)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 19)
vecArray.InsertComponent(idx, 0, 19)
vecArray.InsertComponent(idx, 1, 19)
vecArray.InsertComponent(idx, 2, 19)

cursor.SubdivideLeaf()

# ROOT CELL 4/3/0/[0-3]
cursor.ToChild(0)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 23)
vecArray.InsertComponent(idx, 0, 23)
vecArray.InsertComponent(idx, 1, 23)
vecArray.InsertComponent(idx, 2, 23)
cursor.ToParent()

cursor.ToChild(1)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 24)
vecArray.InsertComponent(idx, 0, 24)
vecArray.InsertComponent(idx, 1, 24)
vecArray.InsertComponent(idx, 2, 24)
cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 25)
vecArray.InsertComponent(idx, 0, 25)
vecArray.InsertComponent(idx, 1, 25)
vecArray.InsertComponent(idx, 2, 25)
cursor.ToParent()

cursor.ToChild(3)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 26)
vecArray.InsertComponent(idx, 0, 26)
vecArray.InsertComponent(idx, 1, 26)
vecArray.InsertComponent(idx, 2, 26)
cursor.ToParent()

# ROOT CELL 4/3/0
cursor.ToParent()

# ROOT CELL 4/3/[1-3]
cursor.ToChild(1)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 20)
vecArray.InsertComponent(idx, 0, 20)
vecArray.InsertComponent(idx, 1, 20)
vecArray.InsertComponent(idx, 2, 20)
cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 21)
vecArray.InsertComponent(idx, 0, 21)
vecArray.InsertComponent(idx, 1, 21)
vecArray.InsertComponent(idx, 2, 21)
cursor.ToParent()

cursor.ToChild(3)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 22)
vecArray.InsertComponent(idx, 0, 22)
vecArray.InsertComponent(idx, 1, 22)
vecArray.InsertComponent(idx, 2, 22)
cursor.ToParent()

# ROOT CELL 4/3

cursor.ToParent()

# ROOT CELL 4

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 5
htg.InitializeNonOrientedCursor(cursor, 5, True)
cursor.SetGlobalIndexStart(offsetIndex)

idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 6)
vecArray.InsertComponent(idx, 0, 6)
vecArray.InsertComponent(idx, 1, 6)
vecArray.InsertComponent(idx, 2, 6)

print('#',scalarArray.GetNumberOfTuples())
print('DataRange: ',scalarArray.GetRange())

print('#',vecArray.GetNumberOfTuples())
print('#',vecArray.GetNumberOfComponents())
print('DataRange: ',vecArray.GetRange())

# Geometries
geometry = vtk.vtkHyperTreeGridGeometry()
geometry.SetInputData(htg)
print('With Geometry Filter (HTG to NS)')

# Shrink Filter
if True:
  print('With Shrink Filter (NS)')
  # En 3D, le shrink ne doit pas se faire sur la geometrie car elle ne represente que la peau
  shrink = vtk.vtkShrinkFilter()
  shrink.SetInputConnection(geometry.GetOutputPort())
  shrink.SetShrinkFactor(.8)
else:
  print('No Shrink Filter (NS)')
  shrink = geometry

# LookupTable
lut = vtk.vtkLookupTable()
lut.SetHueRange(0.66, 0)
lut.UsingLogScale()
lut.Build()

# Mappers
mapper = vtk.vtkDataSetMapper()
mapper.SetInputConnection(shrink.GetOutputPort())

mapper.SetLookupTable(lut)
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray('scalar')
dataRange = [1,26] # Forced for compare with 2DMask
mapper.SetScalarRange(dataRange[0], dataRange[1])

# Actors
actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Camera
bd = htg.GetBounds()
camera = vtk.vtkCamera()
camera.SetClippingRange(1., 100.)
focal = []
for i in range(3):
  focal.append(bd[ 2 * i ] + (bd[ 2 * i + 1 ] - bd[ 2 * i]) / 2.)
camera.SetFocalPoint(focal)
camera.SetPosition(focal[0], focal[1], focal[2] + 4.)

# Renderer
renderer = vtk.vtkRenderer()
renderer.SetActiveCamera(camera)
renderer.AddActor(actor)

# Render window
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(renderer)
renWin.SetSize(600, 400)

# Render window interactor
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# render the image
renWin.Render()
# iren.Start()

# prevent the tk window from showing up then start the event loop
# --- end of script --
