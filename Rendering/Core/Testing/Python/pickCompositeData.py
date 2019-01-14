#!/usr/bin/env python

# This script tests picking props, points and cells rendered with a vtkCompositePolyDataMapper2.
#
import vtk
import math
import time
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create a vtkMultiBlockDataSet with 3
# vtkPolyData blocks, rendered using a vtkCompositePolyDataMapper2
# and picked using vtkPicker, vtkPointPicker and vtkCellPicker.

# points for two 'stacks' of 3 cells, each belonging to a different block
#     first stack                                 second stack
xyz=[[[ 4., 4.],[10., 4.],[10., 6.],[ 4., 6.],    [20., 0.],[25.,0.],[30., 0.],[30.,1.], [25.,1.],[20.,1.]],
     [[ 2., 2.],[12., 2.],[12., 8.],[ 2., 8.],    [22.,-1.],         [28.,-1.],[28.,2.],          [22.,2.]],
     [[ 0., 0.],[14., 0.],[14.,10.],[ 0.,10.],    [24.,-2.],         [26.,-2.],[26.,3.],          [24.,3.]]]

polyconn=[[[0,1,2,3],                             [4,5,8,9],[5,6,7,8]],
          [[0,1,2],[2,3,0],                       [4,5,6],[6,7,4]],
          [[0,1,2,3],                             [4,5,6,7]]]

# These define the tests
# pick-coordinate, prop-pick-expectations, point-pick-expectations, cell-pick-expectations
# prop-pick-expectations: pick-result, block#
# point-pick-expectations: pick-result, block#, point-id
# cell-pick-expectations: pick-result, block#, cell-id, point-id
pickdata=[
  # Pick the first stack
  ([ 0.,0.],(True,3),(True,3,0),(True,3,0,0)),
  ([ 4.,4.],(True,1),(True,1,0),(True,1,0,0)),
  ([ 5.,5.],(True,1),(False,-1,-1),(True,1,0,0)),

  # Pick in between the stacks
  # note: the ray passes between the data sets but the block's bounding box encompasses both data sets
  ([18.,-1.],(True,2),(False,-1,-1),(False,-1,-1,-1)),
  ([18.,-3.],(False,-1),(False,-1,-1),(False,-1,-1,-1)),

  # Pick the second stack
  ([25.,0.],(True,1),(True,1,5),(True,1,1,5)),
  # note: the prop pick hits block #1 because the bounding box of block #1 is picked. The cell picker does not hit a cell of block #1
  ([28.,2.],(True,1),(True,2,6),(True,2,2,6)) ]



# Construct the data-set, and set up color mapping of the blocks
mbd=vtk.vtkMultiBlockDataSet()
mbd.SetNumberOfBlocks(3)
cda=vtk.vtkCompositeDataDisplayAttributes()
m=vtk.vtkCompositePolyDataMapper2()
m.SetInputDataObject(mbd)
m.SetCompositeDataDisplayAttributes(cda)

for blk in range(0,3):
  mbd.SetBlock(blk,vtk.vtkPolyData())
  poly=mbd.GetBlock(blk)
  coords=xyz[blk]
  pts=vtk.vtkPoints()
  for coord in coords:
    pts.InsertNextPoint(coord[0],coord[1],float(blk))

  polys=vtk.vtkCellArray()
  for cell in polyconn[blk]:
     polys.InsertNextCell(len(cell))
     for pid in cell:
       polys.InsertCellPoint(pid)

  poly.SetPoints(pts)
  poly.SetPolys(polys)
  m.SetBlockColor(blk,(blk%3)==0,(blk+1)%3==0,(blk+2)%3==0)

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

# Define the pickers
propPicker = vtk.vtkPicker()
cellPicker = vtk.vtkCellPicker()
pointPicker = vtk.vtkPointPicker()

# this can be switched on to print some debug output
debug=False

def propPick(p,xy,data):
    """
    Pick a prop. camera has been positioned such that a ray pick at
    screen-coordinates p hits points with x-y coordinates xy.
    data contains: expected pick result, block#
    """
    errors = 0
    result=propPicker.Pick(p[0],p[1],0,r)==1
    if result != data[0]:
        print('prop pick at {} result: expected {} actual {}'
                .format(xy,data[0],result))
        errors+=1
    if propPicker.GetFlatBlockIndex() != data[1]:
        print('prop pick at {} flat block index expected {} actual {}'
                .format(xy,data[1],propPicker.GetFlatBlockIndex()))
        errors+=1

    if debug and errors:
        if result>0:
           if propPicker.GetFlatBlockIndex():
                print('prop pick at {} picked block {}'
                        .format(xy,propPicker.GetFlatBlockIndex()))
        else:
            print('prop pick at {} result:{}'.format(xy,result))
    return errors

def pointPick(p,xy,data):
    """
    Pick a prop. camera has been positioned such that a ray pick at
    screen-coordinates p hits points with x-y coordinates xy.
    data contains: expected pick result, block#, point id
    """
    errors = 0
    result=pointPicker.Pick(p[0],p[1],0,r)==1
    if result != data[0]:
        print('point pick at {} result: expected {} actual {}'
                .format(xy,data[0],result))
        errors+=1
    if pointPicker.GetFlatBlockIndex() != data[1]:
        print('point pick at {} flat block index expected {} actual {}'
                .format(xy,data[1],pointPicker.GetFlatBlockIndex()))
        errors+=1
    if pointPicker.GetPointId() != data[2]:
        print('point pick at {} point id expected {} actual {}'
                .format(xy,data[2],pointPicker.GetPointId()))
        errors+=1
    if debug and errors:
        if result>0:
            if pointPicker.GetFlatBlockIndex():
               print('point pick at {} picked point {} of block {}'
                       .format(xy,pointPicker.GetPointId(),pointPicker.GetFlatBlockIndex()))
            else:
                print('point pick at {} picked point {}'
                        .format(xy,pointPicker.GetPointId()))
        else:
            print('point pick at {} result:{}'.format(xy,result))
    return errors

def cellPick(p,xy,data):
    """
    Pick a prop. camera has been positioned such that a ray pick at
    screen-coordinates p hits points with x-y coordinates xy.
    data contains: expected pick result, block#, cell id, point id
    """
    errors=0
    result=cellPicker.Pick(p[0],p[1],0,r)==1
    if result != data[0]:
        print('cell pick at {} result: expected {} actual {}'
                .format(xy,data[0],result))
        errors+=1
    if cellPicker.GetFlatBlockIndex() != data[1]:
        print('cell pick at {} flat block index expected {} actual {}'
                .format(xy,data[1],cellPicker.GetFlatBlockIndex()))
        errors+=1
    if cellPicker.GetCellId() != data[2]:
        print('cell pick at {} cell id expected {} actual {}'
                .format(xy,data[2],cellPicker.GetCellId()))
        errors+=1
    if cellPicker.GetPointId() != data[3]:
        print('cell pick at {} point id expected {} actual {}'
                .format(xy,data[3],cellPicker.GetPointId()))
        errors+=1

    if debug and errors:
        if result>0:
           if cellPicker.GetFlatBlockIndex():
                print('cell pick at {} picked cell {} point {} of block {}'
                        .format(xy,cellPicker.GetCellId(),cellPicker.GetPointId(),cellPicker.GetFlatBlockIndex()))
           else:
                print('cell pick at {} picked cell {} point {}'
                        .format(xy,cellPicker.GetCellId(),cellPicker.GetPointId()))
        else:
            print('cell pick at {} result:{}'.format(xy,result))
    return errors


def pick(xy,propPickData,pointPickData,cellPickData):
    """
    Position the camera such that a ray through the screen center
    points in z direction at position xy, and pick with vtkPicker,
    vtkPointPicker and vtkCellPicker.
    Returns the number of errors
    """
    size=rw.GetSize()
    p=[size[0]/2,size[1]/2,0]

    cam=r.GetActiveCamera()
    cam.SetPosition(xy[0],xy[1],-10.0)
    cam.SetFocalPoint(xy[0],xy[1],0.0)
    cam.SetViewUp(0,1,0)
    cam.SetViewAngle(90)
    cam.ParallelProjectionOff()
    r.ResetCameraClippingRange()
    rw.Render()

    errors = 0

    if debug:
      print('Picking at {}'.format(xy))
    errors += propPick(p,xy,propPickData)
    errors += pointPick(p,xy,pointPickData)
    errors += cellPick(p,xy,cellPickData)

    if debug:
      time.sleep(3)

    return errors


# Run all of the tests defined by pickdata
errors = 0
for data in pickdata:
  errors += pick( data[0], data[1], data[2], data[3] )

if errors:
  print("Encountered {} errors".format(errors))
  sys.exit(1)

# --- end of script --
