# Tests paraview/paraview#18391

from vtkmodules.util.misc import vtkGetDataRoot, vtkGetTempDir
from vtk import vtkXMLGenericDataObjectReader, vtkDoubleArray, vtkDataSetWriter, vtkPDataSetReader

from os.path import join

reader = vtkXMLGenericDataObjectReader()
reader.SetFileName(join(vtkGetDataRoot(), "Data/multicomb_0.vts"))
reader.Update()


a1 = vtkDoubleArray()
a1.SetName("field-1")
a1.SetNumberOfTuples(1)
a1.SetValue(0, 100.0)
a1.GetRange()

a2 = vtkDoubleArray()
a2.SetName("field-2")
a2.SetNumberOfTuples(2)
a2.SetValue(0, 1.0)
a2.SetValue(1, 2.0)
a2.GetRange()

dobj = reader.GetOutputDataObject(0)
dobj.GetFieldData().AddArray(a1)
dobj.GetFieldData().AddArray(a2)

writer = vtkDataSetWriter()
writer.SetFileName(join(vtkGetTempDir(), "TestPDataSetReaderWriterWithFieldData.vtk"))
writer.SetInputDataObject(dobj)
writer.Write()

reader = vtkPDataSetReader()
reader.SetFileName(join(vtkGetTempDir(), "TestPDataSetReaderWriterWithFieldData.vtk"))
reader.Update()

dobj2 = reader.GetOutputDataObject(0)
assert (dobj.GetNumberOfPoints() == dobj2.GetNumberOfPoints() and
        dobj.GetFieldData().GetNumberOfArrays() == dobj2.GetFieldData().GetNumberOfArrays())
