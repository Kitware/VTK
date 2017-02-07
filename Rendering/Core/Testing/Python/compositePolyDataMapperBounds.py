#!/usr/bin/env python

# This script tests the bounds calculation of vtkCompositePolyDataMapper
# when the composite data set has empty blocks.

import vtk
import sys

# If a composite data set had an empty block following one or more non-empty
# blocks, the invalid bounds of the empty block ([1,-1,1,-1,1,-1]) would be
# compared with the current bounds, e.g. -3 is less than -1 hence y_max would
# be -1.
pts = vtk.vtkPoints()
bounds = [2.,4.,-5.,-3.,0.0,0.0]
for y in bounds[2:4]:
  for x in bounds[0:2]:
    pts.InsertNextPoint( x,y,0 )

# One cell
polys=vtk.vtkCellArray()
polys.InsertNextCell(4)
for pid in [0,1,3,2]:
  polys.InsertCellPoint(pid)

# create a polydata with a single quad
pd=vtk.vtkPolyData()
pd.SetPoints(pts)
pd.SetPolys(polys)
pdBounds = pd.GetBounds()

errors = 0
for i in range(0,6):
  if pdBounds[i] != bounds[i]:
    errors +=1
    break

# An empty polydata
empty=vtk.vtkPolyData()

# A composite data set with an empty block following a non-empty block
mbd=vtk.vtkMultiBlockDataSet()
mbd.SetBlock(0,pd)
mbd.SetBlock(1,empty)

# The composite polydata mapper maps the composite dataset
cpdm=vtk.vtkCompositePolyDataMapper()
cpdm.SetInputDataObject(mbd)

# Calculate the bounds
cpdmBounds = cpdm.GetBounds()

ok=True
for i in range(0,6):
  if cpdmBounds[i] != bounds[i]:
    ok=False
    errors+=1
    break

if not ok:
  print( "Empty block following non-empty block" )
  print ( "expected bounds = {}".format(bounds) )
  print ( "polydata bounds = {}".format(pdBounds) )
  print ( "mapper bounds   = {}".format(cpdmBounds) )

if errors:
  sys.exit(1)

# --- end of script ---
