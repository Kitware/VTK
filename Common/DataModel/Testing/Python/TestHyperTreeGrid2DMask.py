#!/usr/bin/env python
"""
Same TestHyperTreeGrid2DFull
with build mask
Warning : only to call SubdivideLeaf on leaf cell don't masked
"""
import vtk

htg = vtk.vtkHyperTreeGrid()
htg.Initialize()

scalarArray = vtk.vtkUnsignedCharArray()
scalarArray.SetName('scalar')
scalarArray.SetNumberOfValues(0)
htg.GetPointData().AddArray(scalarArray)

mask = vtk.vtkBitArray()
mask.SetName('mask')
mask.SetNumberOfValues(26)
mask.FillComponent(0, 0)
htg.SetMask(mask)

htg.SetDimensions([4, 3, 1])
htg.SetBranchFactor(2)

# rectilinear grid coordinates
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

cursor.SubdivideLeaf()

# ROOT CELL 0/[0-3]
cursor.ToChild(0)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 7)
mask.SetValue(idx, 1) # MASK 0/0
cursor.ToParent()

cursor.ToChild(1)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 8)
cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 9)
cursor.ToParent()

cursor.ToChild(3)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 10)
mask.SetValue(idx, 1) # MASK 0/3
cursor.ToParent()

# ROOT CELL 0

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 1
htg.InitializeNonOrientedCursor(cursor, 1, True)
cursor.SetGlobalIndexStart(offsetIndex)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 2)
mask.SetValue(idx, 1) # MASK

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 2
htg.InitializeNonOrientedCursor(cursor, 2, True)
cursor.SetGlobalIndexStart(offsetIndex)

idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 3)

cursor.SubdivideLeaf()

# ROOT CELL 2/[0-3]
cursor.ToChild(0)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 11)
cursor.ToParent()

cursor.ToChild(1)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 12)
cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 13)
mask.SetValue(idx, 1) # MASK 2/2
cursor.ToParent()

cursor.ToChild(3)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 14)
cursor.ToParent()

# ROOT CELL 2
idx = cursor.GetGlobalNodeIndex()
mask.SetValue(idx, 1) # MASK 2 (ROOT CELL)

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 3
htg.InitializeNonOrientedCursor(cursor, 3, True)
cursor.SetGlobalIndexStart(offsetIndex)

idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 4)

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 4
htg.InitializeNonOrientedCursor(cursor, 4, True)
cursor.SetGlobalIndexStart(offsetIndex)

idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 5)

cursor.SubdivideLeaf()

# ROOT CELL 4/[0-3]
cursor.ToChild(0)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 15)
cursor.ToParent()

cursor.ToChild(1)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 16)
mask.SetValue(idx, 1) # MASK 4/1
cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 17)
cursor.ToParent()

cursor.ToChild(3)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 18)

cursor.SubdivideLeaf()

# ROOT CELL 4/3/[0-3]
cursor.ToChild(0)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 19)

cursor.SubdivideLeaf()

# ROOT CELL 4/3/0/[0-3]
cursor.ToChild(0)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 23)
cursor.ToParent()

cursor.ToChild(1)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 24)
cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 25)
mask.SetValue(idx, 1) # MASK 4/3/0/2
cursor.ToParent()

cursor.ToChild(3)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 26)
cursor.ToParent()

# ROOT CELL 4/3/0
cursor.ToParent()

# ROOT CELL 4/3/[1-3]
cursor.ToChild(1)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 20)
cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 21)
mask.SetValue(idx, 1) # MASK 4/3/2
cursor.ToParent()

cursor.ToChild(3)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 22)
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

print('#',scalarArray.GetNumberOfTuples())
print('DataRange: ',scalarArray.GetRange())

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
dataRange = [1,26] # Forced for compare with 2DFull
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
