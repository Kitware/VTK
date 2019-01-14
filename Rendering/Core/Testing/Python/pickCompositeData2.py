#!/usr/bin/env python

# This script demonstrates VTK issue #17211 where the vtkCellPicker returns
# the flat block index not matching the cell and point id.

import vtk
import math
import time
import sys
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# this can be switched on to print some debug output
debug=False

# A regular 3x3x3 grid of points
dx=2.
dy=3.
xyz=list()
for z in [ -4., -2., 0. ]:
    for y in [ 0., dy, 2*dy ]:
        for x in [ 0., dx, 2*dx ]:
            xyz.append([x,y,z])

# Create a vtkMultiBlockDataSet with 4  vtkPolyData blocks, each
# with some vertices an a single quad.
# The vertex cells are added to give the polygon cell which is the
# last cell in each polydata a different
parts=[
        {'coordinates':xyz,
         'verts':[[v] for v in range(0,4)],
         'polys':[[0,1,4,3]], # cell id 4
         'color':[1,0,0] },
        {'coordinates':xyz,
         'verts':[[v] for v in range(0,3)],
         'polys':[[1,2,5,4]], # cell id 3
         'color':[0,1,0] },
        {'coordinates':xyz,
         'verts':[[v] for v in range(0,2)],
         'polys':[[3,4,7,6]], # cell id 2
         'color':[0,0,1] },
        {'coordinates':xyz,
         'verts':[[v] for v in range(0,1)],
         'polys':[[4,5,8,7]], # cell id 1
         'color':[1,1,0] },
        ]

# Construct the data-set, and set up color mapping of the blocks
mbd=vtk.vtkMultiBlockDataSet()
mbd.SetNumberOfBlocks(len(parts))
cda=vtk.vtkCompositeDataDisplayAttributes()
m=vtk.vtkCompositePolyDataMapper2()
m.SetInputDataObject(mbd)
m.SetCompositeDataDisplayAttributes(cda)

for p in range(0,len(parts)):

    points=vtk.vtkPoints()
    verts=vtk.vtkCellArray()
    polys=vtk.vtkCellArray()

    poly=vtk.vtkPolyData()
    poly.SetPoints(points)
    poly.SetVerts(verts)
    poly.SetPolys(polys)

    coords=parts[p]['coordinates']
    for coord in coords:
        points.InsertNextPoint(coord)

    # Iterate over the connectivity descriptions and fill
    # the corresponding cell arrays
    for connectivity in zip( [ parts[p]['verts'], parts[p]['polys'] ],
                             [ verts,             polys             ] ):
        for cell in connectivity[0]:
            connectivity[1].InsertNextCell(len(cell))
            for pid in cell:
                connectivity[1].InsertCellPoint(pid)

    # Assign the block
    mbd.SetBlock(p,poly)
    f=float(p+1)/len(parts)
    m.SetBlockColor(p+1,parts[p]['color'])
    if debug:
        print( "block {0} bounds {1} #pts={2} #cells={3}"
                .format(p+1,
                        poly.GetBounds(),
                        poly.GetNumberOfPoints(),
                        poly.GetNumberOfCells()))

# Set up the actor
a=vtk.vtkActor()
a.SetMapper(m)
a.GetProperty().EdgeVisibilityOn()
a.GetProperty().SetEdgeColor(1,1,1)

# Render the actor
r = vtk.vtkRenderer()
r.AddViewProp(a)
r.SetBackground(0,0,0)
rw = vtk.vtkRenderWindow()
rw.AddRenderer(r)
rw.Render()

# Define the picker
cellPicker = vtk.vtkCellPicker()

# These define the tests
#
# tuple layouts of pickdata:
# (pick-coordinate, cell-pick-expectations)
# cell-pick-expectations tuple: (pick-result, flat block index,cell#,point#)
delta=0.00001
pickdata=[
  ((.1*dx,.1*dy),(True,1,4,0)),
  ((.9*dx,.9*dy),(True,1,4,4)),
  # the following pick position is close enough to also hit block #3
  ((.9*dx,dy-delta),(True,3,2,4)),
  ((.9*dx,dy+delta),(True,3,2,4)),
  ]

def expectedResult(data):
    """Returns whether pick is expected to be successful"""
    return data[0]

def expectedBlockId(data):
    """Returns the expected flat block index"""
    return data[1]

def expectedCellId(data):
    """ Returns the expected cell index """
    if not data[0] or data[1]<0:
        return -1
    return data[2]


def compareExpectedAndActual(what,expected,actual):
    if actual != expected:
        print( '{0} expected: {1} actual: {2}'
                .format(what,expected,actual) )
        return 1
    return 0

def reportPickResult(what,result,block=-1,cell=-1,point=-1):
    text=what
    if not result:
        text += ' result: {0}'.format(result)
    else:
        if block>-1:
            text += ' block {0}'.format(block)
        if cell>-1:
            text += ' cell {0}'.format(cell)
        if point>-1:
            text += ' point {0}'.format(point)
    print(text)

def cellPick(p,xy,data):
    """
    Pick a prop. camera has been positioned such that a ray pick at
    screen-coordinates p hits points with x-y coordinates xy.
    data tuple contents: (expected_result,block#,cell#,point#)
    """
    errors=0
    what='cell pick at {0}'.format(xy)
    result=cellPicker.Pick(p[0],p[1],0,r)==1
    errors += compareExpectedAndActual( '{0} result'.format(what),
                                        expectedResult(data),
                                        result )

    errors += compareExpectedAndActual( '{0} flat block index'.format(what),
                                        expectedBlockId(data),
                                        cellPicker.GetFlatBlockIndex())

    errors += compareExpectedAndActual( '{0} cell id'.format(what),
                                        expectedCellId(data),
                                        cellPicker.GetCellId())

    if errors and debug:
        reportPickResult(what,result>0,
                         block=cellPicker.GetFlatBlockIndex(),
                         cell=cellPicker.GetCellId(),
                         point=cellPicker.GetPointId())
    return errors


def pick(xy,cellPickData):
    """
    Position the camera such that a ray through the screen center
    points in z direction at position xy, and pick with vtkPicker,
    vtkPointPicker and vtkCellPicker.
    Returns the number of errors
    """
    size=rw.GetSize()
    p=[size[0]/2,size[1]/2,0]

    cam=r.GetActiveCamera()
    cam.SetPosition(xy[0],xy[1],2.0)
    cam.SetFocalPoint(xy[0],xy[1],0.0)
    cam.SetViewUp(0,1,0)
    cam.SetViewAngle(35)
    cam.ParallelProjectionOn()
    r.ResetCameraClippingRange()
    rw.Render()

    if debug:
      print('Picking at {0}'.format(xy))

    errors = cellPick(p,xy,cellPickData)

    if debug:
      time.sleep(2)

    return errors


# Run all of the tests defined by pickdata
errors = 0
for data in pickdata:
    errors += pick( data[0], data[1] )

if errors:
  print("Encountered {} errors".format(errors))
  sys.exit(1)

# --- end of script --
