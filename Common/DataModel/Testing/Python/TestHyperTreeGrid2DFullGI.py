#!/usr/bin/env python
"""
Create a HTG
without mask
build scalar before
used Global Index Map explicit with SetGlobalIndexFromLocal
SetGlobalIndexFromLocal, one call by cell
"""
import vtk

htg = vtk.vtkHyperTreeGrid()
htg.Initialize()

scalarArray = vtk.vtkUnsignedCharArray()
scalarArray.SetName('scalar')
scalarArray.SetNumberOfValues(26)
htg.GetPointData().AddArray(scalarArray)
htg.GetPointData().SetActiveScalars('scalar')

for i in range(26):
  scalarArray.InsertTuple1(i, i+1)

htg.SetDimensions([4, 3, 1])
htg.SetBranchFactor(2)

# rectilinear grid coordinates
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
zValues.SetNumberOfValues(1)
zValues.SetValue(0, 0)
htg.SetZCoordinates(zValues);

# Let's split the various trees
cursor = vtk.vtkHyperTreeGridNonOrientedCursor()

# ROOT CELL 0
htg.InitializeNonOrientedCursor(cursor, 0, True)
cursor.SetGlobalIndexFromLocal(0)

cursor.SubdivideLeaf()

# ROOT CELL 0/[0-3]
cursor.ToChild(0)
cursor.SetGlobalIndexFromLocal(6)
cursor.ToParent()

cursor.ToChild(1)
cursor.SetGlobalIndexFromLocal(7)
cursor.ToParent()

cursor.ToChild(2)
cursor.SetGlobalIndexFromLocal(8)
cursor.ToParent()

cursor.ToChild(3)
cursor.SetGlobalIndexFromLocal(9)
cursor.ToParent()

# ROOT CELL 1
htg.InitializeNonOrientedCursor(cursor, 1, True)
cursor.SetGlobalIndexFromLocal(1)

# ROOT CELL 2
htg.InitializeNonOrientedCursor(cursor, 2, True)
cursor.SetGlobalIndexFromLocal(2)

cursor.SubdivideLeaf()

# ROOT CELL 2/[0-3]
cursor.ToChild(0)
cursor.SetGlobalIndexFromLocal(10)
cursor.ToParent()

cursor.ToChild(1)
cursor.SetGlobalIndexFromLocal(11)
cursor.ToParent()

cursor.ToChild(2)
cursor.SetGlobalIndexFromLocal(12)
cursor.ToParent()

cursor.ToChild(3)
cursor.SetGlobalIndexFromLocal(13)

# ROOT CELL 3
htg.InitializeNonOrientedCursor(cursor, 3, True)
cursor.SetGlobalIndexFromLocal(3)

# ROOT CELL 4
htg.InitializeNonOrientedCursor(cursor, 4, True)
cursor.SetGlobalIndexFromLocal(4)

cursor.SubdivideLeaf()

# ROOT CELL 4/[0-3]
cursor.ToChild(0)
cursor.SetGlobalIndexFromLocal(14)
cursor.ToParent()

cursor.ToChild(1)
cursor.SetGlobalIndexFromLocal(15)
cursor.ToParent()

cursor.ToChild(2)
cursor.SetGlobalIndexFromLocal(16)
cursor.ToParent()

cursor.ToChild(3)
cursor.SetGlobalIndexFromLocal(17)

cursor.SubdivideLeaf()

# ROOT CELL 4/3/[0-3]
cursor.ToChild(0)
cursor.SetGlobalIndexFromLocal(18)

cursor.SubdivideLeaf()

# ROOT CELL 4/3/0/[0-3]
cursor.ToChild(0)
cursor.SetGlobalIndexFromLocal(22)
cursor.ToParent()

cursor.ToChild(1)
cursor.SetGlobalIndexFromLocal(23)
cursor.ToParent()

cursor.ToChild(2)
cursor.SetGlobalIndexFromLocal(24)
cursor.ToParent()

cursor.ToChild(3)
cursor.SetGlobalIndexFromLocal(25)
cursor.ToParent()

# ROOT CELL 4/3/0
cursor.ToParent()

# ROOT CELL 4/3/[1-3]
cursor.ToChild(1)
cursor.SetGlobalIndexFromLocal(19)
cursor.ToParent()

cursor.ToChild(2)
cursor.SetGlobalIndexFromLocal(20)
cursor.ToParent()

cursor.ToChild(3)
cursor.SetGlobalIndexFromLocal(21)

# ROOT CELL 5
htg.InitializeNonOrientedCursor(cursor, 5, True)
cursor.SetGlobalIndexFromLocal(5)

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
