#!/usr/bin/env python

from vtk import *
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

rdr = vtkExodusIIReader()
rdr.SetFileName(str(VTK_DATA_ROOT) + "/Data/shared_face_polyhedra.exo")
rdr.Update()

srf = vtkDataSetSurfaceFilter()
srf.SetInputConnection(rdr.GetOutputPort())
srf.Update()

aa = vtkAssignAttribute()
aa.SetInputConnection(srf.GetOutputPort())
aa.Assign('ObjectId', vtkDataSetAttributes.SCALARS, vtkAssignAttribute.CELL_DATA)
aa.Update()

renWin = vtkRenderWindow()
ri = vtkRenderWindowInteractor()
rr = vtkRenderer()
rr.SetBackground(1, 1, 1)
ac = vtkActor()
dm = vtkCompositePolyDataMapper()
ac.SetMapper(dm)
dm.SetInputConnection(aa.GetOutputPort())
dm.SetScalarVisibility(True)
#dm.SetScalarModeToUseCellFieldData()
#dm.SelectColorArray('ObjectId')
#dm.ColorByArrayComponent('ObjectId', 0)
dm.SetScalarRange([10000, 20000])
dm.UseLookupTableScalarRangeOff()
rr.AddActor(ac)
renWin.AddRenderer(rr)
renWin.SetInteractor(ri)
rr.GetActiveCamera().SetPosition(4.45025631439989, -0.520617222824798, 1.08873910981941)
rr.GetActiveCamera().SetFocalPoint(-0.441087924633898, 1.43633923685504, -1.32653110659694)
rr.GetActiveCamera().SetViewUp(-0.424967955165741, 0.0530900205407349, 0.903650201572065)
rr.ResetCamera()
renWin.Render()
#ri.Start()
