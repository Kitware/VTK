#!/usr/bin/env python

from vtk import *
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

rdr = vtkExodusIIReader()
rdr.SetFileName(str(VTK_DATA_ROOT) + "/Data/dodecahedron.exo")
rdr.Update()

ph = rdr.GetOutput().GetBlock(0).GetBlock(0).GetCell(0)
print '%d polyhedral faces' % ph.GetNumberOfFaces()
if ph.GetNumberOfFaces() != 12:
  sys.exit(1)
for i in range(ph.GetNumberOfFaces()):
  pg = ph.GetFace(i)
  if pg.GetNumberOfEdges() != 5 or pg.GetNumberOfPoints() != 5:
    print '  %d edges on face %d' % (pg.GetNumberOfEdges(), i)
    print '  %d points on face %d' % (pg.GetNumberOfPoints(), i)
    sys.exit(1)
  #for j in range(pg.GetNumberOfPoints()):
  #  pid = pg.GetPointId(j)
  #  x = [0.0,0.0,0.0]
  #  pg.GetPoints().GetPoint(pid, x)
  #  print '   p%02d: %g %g %g' % (pid, x[0], x[1], x[2]);

renWin = vtkRenderWindow()
ri = vtkRenderWindowInteractor()
rr = vtkRenderer()
ac = vtkActor()
dm = vtkDataSetMapper()
ac.SetMapper(dm)
dm.SetInputData(rdr.GetOutput().GetBlock(0).GetBlock(0))
rr.AddActor(ac)
renWin.AddRenderer(rr)
rr.ResetCamera()
renWin.Render()
