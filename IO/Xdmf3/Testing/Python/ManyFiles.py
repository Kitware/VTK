"""
This test verifies that vtk's Xdmf reader will read a sampling of small
to moderate size data files that cover a spectrum of file format features.
"""

from __future__ import print_function

import sys
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

smallFiles = [
#from XDMF
"2DCoRectMesh.xmf",
"2DRectMesh.xmf",
"2DSMesh.xmf",
"3DCoRectMesh.xmf",
"3DRectMesh.xmf",
"3DSMesh.xmf",
"Graph.xmf",
"Hexahedron.xmf",
"HexahedronTimestep.xmf",
"Mixed.xmf",
"Tetrahedron.xmf",
"TetrahedronMultipleGrids.xmf",
"TetrahedronSpatial.xmf",
"TetrahedronSpatialTimestep.xmf",
"TetrahedronTimestep.xmf",
"Triangle.xmf",
"corect.xmf",
"hex20.xmf",
"hexahedron_xy.xmf",
"hexahedron_xyz.xmf",
"output.xmf",
"quadrilateral.xmf",
"rectTest.xmf",
"tensor.xmf",
"set.xmf"
]

largeFiles = [
#from ParaView
"Iron/Iron_Protein.ImageData.xmf",
"Iron/Iron_Protein.ImageData.Collection.xmf",
"Iron/Iron_Protein.RectilinearGrid.xmf",
"Iron/Iron_Protein.RectilinearGrid.Collection.xmf",
"Iron/Iron_Protein.StructuredGrid.xmf",
"Iron/Iron_Protein.StructuredGrid.Collection.xmf",
"Big/Scenario1_p1.xmf",
]

testfilenames = smallFiles

import sys
if "--do_big_files" in sys.argv:
  testfilenames = smallFiles + largeFiles

if __name__ == "__main__":
  for fname in testfilenames:
    xr = vtk.vtkXdmf3Reader()
    afname = "" + str(VTK_DATA_ROOT) + "/Data/XDMF/" + fname
    print ("Trying %s" % afname)
    xr.CanReadFile(afname)
    xr.SetFileName(afname)
    xr.UpdateInformation()
    xr.Update()
    ds = xr.GetOutputDataObject(0)
    if not ds:
      print("Got zero output from known good file")
      sys.exit(vtk.VTK_ERROR)
    #if res != vtk.VTK_OK:
    #  print "Could not read", afname
    #  sys.exit(vtk.VTK_ERROR)
