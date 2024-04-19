#!/usr/bin/env python

from vtkmodules.vtkCommonDataModel import vtkDataSetAttributes
from vtkmodules.vtkFiltersCore import vtkAssignAttribute
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkIOEnSight import vtkGenericEnSightReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCompositePolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

rdr = vtkGenericEnSightReader()
rdr.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/pitzDaily.case")
rdr.Update()

srf = vtkDataSetSurfaceFilter()
srf.SetInputConnection(rdr.GetOutputPort())
srf.Update()

aa = vtkAssignAttribute()
aa.SetInputConnection(srf.GetOutputPort())
aa.Assign('UGrad', vtkDataSetAttributes.SCALARS, vtkAssignAttribute.CELL_DATA)
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
dm.SetScalarRange([0, 100])
dm.UseLookupTableScalarRangeOff()
rr.AddActor(ac)
renWin.AddRenderer(rr)
renWin.SetInteractor(ri)
rr.ResetCamera()
renWin.Render()
#ri.Start()
