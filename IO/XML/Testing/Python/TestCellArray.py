#!/usr/bin/env python

import os
import vtk
from vtk.util.misc import vtkGetTempDir

VTK_TEMP_DIR = vtkGetTempDir()
filename = VTK_TEMP_DIR + '/TestCellArray.vtu'


ugrid = vtk.vtkUnstructuredGrid()
pts = vtk.vtkPoints()
ugrid.SetPoints(pts)
ugrid.Allocate(10)

for i in range(10):
  pts.InsertNextPoint(i, i, i)
  ids = [i]
  ugrid.InsertNextCell(1, 1, ids) # a vertex

writer = vtk.vtkXMLUnstructuredGridWriter()
writer.SetInputDataObject(ugrid)
writer.SetFileName(filename)
writer.Write()

reader = vtk.vtkXMLUnstructuredGridReader()
reader.SetFileName(filename)
reader.Update()

readergrid = reader.GetOutput()
copygrid = readergrid.NewInstance()

copygrid.DeepCopy(readergrid)

pts = copygrid.GetPoints()
#o.SetPoints(pts)

pt = [1, 2, 3]

idlist = vtk.vtkIdList()
idlist.SetNumberOfIds(1)

for i in range(5):
  newpt = [10+i, 10+i, 10+i]
  pts.InsertNextPoint(newpt)
  iii = [copygrid.GetNumberOfPoints()-1]
  copygrid.InsertNextCell(1, 1, iii)
  copygrid.GetCellPoints(copygrid.GetNumberOfCells()-1, idlist)

if copygrid.GetNumberOfPoints() != 15 or copygrid.GetNumberOfCells() != 15:
  print ("ERROR: incorrect number of points and or cells")
  import sys
  sys.exit(1)

count = 0
ci = copygrid.NewCellIterator()
# Now we verify that each vertex has the proper point (the cell id should be the point id)
while ci.IsDoneWithTraversal() == False:
  id = ci.GetCellId()
  ids = ci.GetPointIds()
  copygrid.GetCellPoints(id, idlist)
  if ids.GetId(0) != idlist.GetId(0) or idlist.GetId(0) != count:
    print ("ERROR: incorrect cell points ", count, id, ids.GetId(0), idlist.GetId(0))
    import sys
    sys.exit(1)

  count = count+1
  ci.GoToNextCell()

ci.UnRegister(None)
ci = None

os.remove(filename)
