#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkDataObject
from vtkmodules.vtkFiltersGeneral import vtkSynchronizeTimeFilter
from vtkmodules.vtkFiltersHybrid import vtkTemporalShiftScale
from vtkmodules.vtkIOExodus import vtkExodusIIReader
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

r = vtkExodusIIReader()
r.SetFileName(VTK_DATA_ROOT + '/Data/can.ex2')

tss = vtkTemporalShiftScale()
tss.SetInputConnection(r.GetOutputPort())

tss.SetPreShift(.00000001)

st = vtkSynchronizeTimeFilter()
st.SetRelativeTolerance(.001)
assert st.GetRelativeTolerance() == .001

st.SetInputConnection(tss.GetOutputPort())
st.SetInputConnection(1, r.GetOutputPort())

st.UpdateTimeStep(0)

ds = st.GetOutput()
firstTimeStepValue = ds.GetInformation().Get(vtkDataObject.DATA_TIME_STEP())

assert firstTimeStepValue == 0.
