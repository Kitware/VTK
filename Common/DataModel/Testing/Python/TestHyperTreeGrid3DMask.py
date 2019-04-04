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

mask = vtk.vtkBitArray()
mask.SetName('mask')
mask.SetNumberOfValues(52)
mask.FillComponent(0, 0)
htg.SetMask(mask)

htg.SetDimensions([4, 3, 3])
htg.SetBranchFactor(2)

# Rectilinear grid coordinates
xValues = vtk.vtkDoubleArray()
xValues.SetNumberOfValues(4)
xValues.SetValue(0, -1)
xValues.SetValue(1, 0)
xValues.SetValue(2, 1)
xValues.SetValue(3, 2)
htg.SetXCoordinates(xValues);

yValues = vtk.vtkDoubleArray()
yValues.SetNumberOfValues(3)
yValues.SetValue(0, -1)
yValues.SetValue(1, 0)
yValues.SetValue(2, 1)
htg.SetYCoordinates(yValues);

zValues = vtk.vtkDoubleArray()
zValues.SetNumberOfValues(4)
zValues.SetValue(0, -1)
zValues.SetValue(1, 0)
zValues.SetValue(2, 1)
zValues.SetValue(3, 2)
htg.SetZCoordinates(zValues);

# Let's split the various trees
cursor = vtk.vtkHyperTreeGridNonOrientedCursor()
offsetIndex = 0

# ROOT CELL 0-4
for iHT in range(5):
  htg.InitializeNonOrientedCursor(cursor, iHT, True)
  cursor.SetGlobalIndexStart(offsetIndex)
  idx = cursor.GetGlobalNodeIndex()
  scalarArray.InsertTuple1(idx, iHT+1)
  offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 5
htg.InitializeNonOrientedCursor(cursor, 5, True)
cursor.SetGlobalIndexStart(offsetIndex)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 6)
mask.SetValue(idx, 1) # MASK 5
offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 6
htg.InitializeNonOrientedCursor(cursor, 6, True)
cursor.SetGlobalIndexStart(offsetIndex)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 7)
cursor.SubdivideLeaf()

# ROOT CELL 6/[0-7]
for ichild in range(8):
  cursor.ToChild(ichild)
  idx = cursor.GetGlobalNodeIndex()
  scalarArray.InsertTuple1(idx, 13+ichild)
  cursor.ToParent()

idx = cursor.GetGlobalNodeIndex()
mask.SetValue(idx, 1) # MASK 6

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 7
htg.InitializeNonOrientedCursor(cursor, 7, True)
cursor.SetGlobalIndexStart(offsetIndex)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 8)

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 8
htg.InitializeNonOrientedCursor(cursor, 8, True)
cursor.SetGlobalIndexStart(offsetIndex)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 9)

cursor.SubdivideLeaf()

# ROOT CELL 8/[0-2]
for ichild in range(2):
  cursor.ToChild(ichild)
  idx = cursor.GetGlobalNodeIndex()
  scalarArray.InsertTuple1(idx, 21+ichild)
  cursor.ToParent()

cursor.ToChild(2)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 21+2)
mask.SetValue(idx, 1) # MASK 8/2
cursor.ToParent()

for ichild in range(3, 8):
  cursor.ToChild(ichild)
  idx = cursor.GetGlobalNodeIndex()
  scalarArray.InsertTuple1(idx, 21+ichild)
  cursor.ToParent()

idx = cursor.GetGlobalNodeIndex()
mask.SetValue(idx, 1) # MASK 8

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 9
htg.InitializeNonOrientedCursor(cursor, 9, True)
cursor.SetGlobalIndexStart(offsetIndex)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 10)

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 10
htg.InitializeNonOrientedCursor(cursor, 10, True)
cursor.SetGlobalIndexStart(offsetIndex)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 11)

cursor.SubdivideLeaf()

# ROOT CELL 10/[0-7]
for ichild in range(8):
  cursor.ToChild(ichild)
  idx = cursor.GetGlobalNodeIndex()
  scalarArray.InsertTuple1(idx, 29+ichild)
  cursor.ToParent()

cursor.ToChild(7)
cursor.SubdivideLeaf()
# ROOT CELL 4/3/[0-3]
for ichild in range(8):
  cursor.ToChild(ichild)
  idx = cursor.GetGlobalNodeIndex()
  scalarArray.InsertTuple1(idx, 37+ichild)
  if ichild == 3:
    mask.SetValue(idx, 1) # MASK 4/3/3
  cursor.ToParent()

cursor.ToChild(4)
cursor.SubdivideLeaf()

# ROOT CELL 4/3/0/[0-3]
for ichild in range(8):
  cursor.ToChild(ichild)
  idx = cursor.GetGlobalNodeIndex()
  scalarArray.InsertTuple1(idx, 46+ichild)
  if ichild == 4 or ichild == 5:
    mask.SetValue(idx, 1) # MASK 4/3/4/5-5
  cursor.ToParent()

offsetIndex += cursor.GetTree().GetNumberOfVertices()

# ROOT CELL 11
htg.InitializeNonOrientedCursor(cursor, 11, True)
cursor.SetGlobalIndexStart(offsetIndex)
idx = cursor.GetGlobalNodeIndex()
scalarArray.InsertTuple1(idx, 12)

print('#',scalarArray.GetNumberOfTuples())
print('DataRange: ',scalarArray.GetRange())

# Geometries
geometry = vtk.vtkHyperTreeGridGeometry()
geometry.SetInputData(htg)
print('With Geometry Filter (HTG to NS)')

# LookupTable
lut = vtk.vtkLookupTable()
lut.SetHueRange(0.66, 0)
lut.UsingLogScale()
lut.Build()

# Mappers
mapper = vtk.vtkDataSetMapper()
mapper.SetInputConnection(geometry.GetOutputPort())

mapper.SetLookupTable(lut)
mapper.SetColorModeToMapScalars()
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray('scalar')
dataRange = [1,53] # Forced for compare with 3DMask
mapper.SetScalarRange(dataRange[0], dataRange[1])

# Actors
actor1 = vtk.vtkActor()
actor1.SetMapper(mapper)

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper)
actor2.GetProperty().SetColor(0, 0, 0)
actor2.GetProperty().SetRepresentationToWireframe()

# Camera
bd = htg.GetBounds()
camera = vtk.vtkCamera()
camera.SetClippingRange(1., 100.)
focal = []
for i in range(3):
  focal.append(bd[ 2 * i ] + (bd[ 2 * i + 1 ] - bd[ 2 * i]) / 2.)
camera.SetFocalPoint(focal)
camera.SetPosition(focal[0]+4, focal[1]+3, focal[2] + 6.)

# Renderer
renderer = vtk.vtkRenderer()
renderer.SetActiveCamera(camera)
renderer.AddActor(actor1)
renderer.AddActor(actor2)

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
