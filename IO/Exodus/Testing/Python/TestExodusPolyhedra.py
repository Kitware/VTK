#!/usr/bin/env python

from vtk import *
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

rdr = vtkExodusIIReader()
rdr.SetFileName(str(VTK_DATA_ROOT) + "/Data/dodecahedron.exo")
rdr.Update()

tfm = vtkTransformFilter()
xfm = vtkTransform()
xfm.Translate(1.5, 0.5, 0.5)
xfm.Scale(0.1, 0.1, 0.1)
tfm.SetInputData(rdr.GetOutput().GetBlock(0).GetBlock(0))
tfm.SetTransform(xfm)
tfm.Update()

rd2 = vtkExodusIIReader()
rd2.SetFileName(str(VTK_DATA_ROOT) + "/Data/cube-1.exo")
rd2.Update()

shr = vtkShrinkFilter()
shr.SetInputData(rd2.GetOutput().GetBlock(0).GetBlock(0))
shr.Update()

ph = rdr.GetOutput().GetBlock(0).GetBlock(0).GetCell(0)
print('%d polyhedral faces' % ph.GetNumberOfFaces())
if ph.GetNumberOfFaces() != 12:
  sys.exit(1)
for i in range(ph.GetNumberOfFaces()):
  pg = ph.GetFace(i)
  if pg.GetNumberOfEdges() != 5 or pg.GetNumberOfPoints() != 5:
    print('  %d edges on face %d' % (pg.GetNumberOfEdges(), i))
    print('  %d points on face %d' % (pg.GetNumberOfPoints(), i))
    sys.exit(1)
  #for j in range(pg.GetNumberOfPoints()):
  #  pid = pg.GetPointId(j)
  #  x = [0.0,0.0,0.0]
  #  pg.GetPoints().GetPoint(pid, x)
  #  print('   p%02d: %g %g %g' % (pid, x[0], x[1], x[2]))

renWin = vtkRenderWindow()
ri = vtkRenderWindowInteractor()
rr = vtkRenderer()
ac = vtkActor()
dm = vtkDataSetMapper()
ac.SetMapper(dm)
dm.SetInputData(tfm.GetOutput())
rr.AddActor(ac)
a2 = vtkActor()
d2 = vtkDataSetMapper()
a2.SetMapper(d2)
d2.SetInputData(shr.GetOutput())
rr.AddActor(a2)
renWin.AddRenderer(rr)
renWin.SetInteractor(ri)
rr.GetActiveCamera().SetPosition(2.09, 1.419, 3.32)
rr.GetActiveCamera().SetFocalPoint(0.838, 0.431, 0.431)
rr.GetActiveCamera().SetViewUp(0.0820, 0.934, -0.348)
rr.ResetCamera()
renWin.Render()
