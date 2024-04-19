#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkIdFilter
from vtkmodules.vtkFiltersGeneral import vtkDataSetTriangleFilter
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import vtkImageClip
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


def addGlobalIds(dataset):
    ids = vtkIdFilter()
    ids.PointIdsOn()
    ids.CellIdsOn()
    ids.SetPointIdsArrayName("vtkGlobalIds")
    ids.SetCellIdsArrayName("vtkGlobalIds")
    ids.SetInputDataObject(dataset)
    ids.Update()

    data2 = ids.GetOutput()
    dataset.GetCellData().SetGlobalIds(data2.GetCellData().GetArray("vtkGlobalIds"))
    dataset.GetPointData().SetGlobalIds(data2.GetPointData().GetArray("vtkGlobalIds"))

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,64)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
reader.SetDataSpacing(1.6,1.6,1.5)
clipper = vtkImageClip()
clipper.SetInputConnection(reader.GetOutputPort())
clipper.SetOutputWholeExtent(30,36,30,36,30,36)

tris = vtkDataSetTriangleFilter()
tris.SetInputConnection(clipper.GetOutputPort())
tris.UpdatePiece(0, 8, 1);

ugdata = tris.GetOutput()
addGlobalIds(ugdata)

# confirm global ids are preserved for unstructured grid
assert ugdata.IsA("vtkUnstructuredGrid")
assert ugdata.GetCellData().GetGlobalIds() != None and ugdata.GetPointData().GetGlobalIds() != None
ugdata.RemoveGhostCells()
assert ugdata.GetCellData().GetGlobalIds() != None and ugdata.GetPointData().GetGlobalIds() != None

# confirm global ids are preserved for poly data
geom = vtkGeometryFilter()
geom.SetInputConnection(tris.GetOutputPort())
geom.UpdatePiece(0, 8, 1);

pddata = geom.GetOutput()
addGlobalIds(pddata)

# confirm global ids are preserved for unstructured grid
assert pddata.IsA("vtkPolyData")
assert pddata.GetCellData().GetGlobalIds() != None and pddata.GetPointData().GetGlobalIds() != None
pddata.RemoveGhostCells()
assert pddata.GetCellData().GetGlobalIds() != None and pddata.GetPointData().GetGlobalIds() != None
