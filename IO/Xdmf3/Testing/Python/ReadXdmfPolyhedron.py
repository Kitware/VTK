"""
This test verifies that vtk's Xdmf reader will read a polyhedron sample file.
"""

from __future__ import print_function

import os, sys
from vtkmodules.vtkCommonCore import VTK_ERROR
from vtkmodules.vtkIOXdmf3 import vtkXdmf3Reader
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

if __name__ == "__main__":
    fnameIn = VTK_DATA_ROOT + "/Data/XDMF/polyhedron.xmf"

    xr = vtkXdmf3Reader()
    xr.CanReadFile(fnameIn)
    xr.SetFileName(fnameIn)
    xr.UpdateInformation()
    xr.Update()
    ds = xr.GetOutputDataObject(0)
    if not ds:
        print("Got zero output from known good file")
        sys.exit(VTK_ERROR)

    print(ds.GetCells())
    print(ds.GetPoints())
