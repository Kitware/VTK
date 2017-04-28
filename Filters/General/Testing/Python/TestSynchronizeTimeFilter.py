#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

r = vtk.vtkExodusIIReader()
r.SetFileName(VTK_DATA_ROOT + '/Data/can.ex2')

tss = vtk.vtkTemporalShiftScale()
tss.SetInputConnection(r.GetOutputPort())

tss.SetPreShift(.00000001)

st = vtk.vtkSynchronizeTimeFilter()
st.SetRelativeTolerance(.001)
assert st.GetRelativeTolerance() == .001

st.SetInputConnection(tss.GetOutputPort())
st.SetInputConnection(1, r.GetOutputPort())

st.UpdateTimeStep(0)

ds = st.GetOutput()
firstTimeStepValue = ds.GetInformation().Get(vtk.vtkDataObject.DATA_TIME_STEP())

assert firstTimeStepValue == 0.
