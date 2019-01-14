#!/usr/bin/env python
# This test loads an Exodus file with NaNs and we test that the vtkDataArray
# returns a correct range for the array with NaNs i.e. not including the NaN.
from vtk import *
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

rdr = vtkExodusIIReader()
rdr.SetFileName(str(VTK_DATA_ROOT) + "/Data/cyl_with_NaN.g")
rdr.UpdateInformation()
rdr.SetPointResultArrayStatus("dist_from_origin", 1);
rdr.Update()

data = rdr.GetOutput().GetBlock(0).GetBlock(0)

# Test that this dataset with NaNs gets a correct range i.e. range without NaNs
# in it.
drange = data.GetPointData().GetArray("dist_from_origin").GetRange()[:]
print("'dist_from_origin' Range: ", drange)
assert (abs(drange[0] - 0.5) < 1e-3) and (abs(drange[1] - 1.118) < 1e-3)
