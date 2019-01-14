import vtk
import sys

testing = vtk.vtkTesting()
for arg in sys.argv:
    testing.AddArgument(arg)
VTK_DATA_ROOT = testing.GetDataRoot()

def Test1(datadir):
    reader = vtk.vtkXMLUnstructuredGridReader()
    reader.SetFileName(datadir + "/Data/quadraticTetra01.vtu")
    reader.UpdateInformation();
    reader.GetPointDataArraySelection().EnableAllArrays()
    reader.GetCellDataArraySelection().EnableAllArrays()

    f = vtk.vtkIntegrateAttributes()
    f.SetInputConnection(reader.GetOutputPort())
    f.Update()

    result = f.GetOutputDataObject(0)
    val = result.GetPointData().GetArray("scalars").GetValue(0)
    assert (val > 0.0162 and val < 0.01621)

    val = result.GetCellData().GetArray("Volume").GetValue(0)
    assert (val > 0.128 and val < 0.1284)


def Test2(datadir):
    reader = vtk.vtkXMLUnstructuredGridReader()
    reader.SetFileName(datadir + "/Data/elements.vtu")
    reader.UpdateInformation();
    reader.GetPointDataArraySelection().EnableAllArrays()
    reader.GetCellDataArraySelection().EnableAllArrays()

    f = vtk.vtkIntegrateAttributes()
    f.SetInputConnection(reader.GetOutputPort())
    f.Update()

    result = f.GetOutputDataObject(0)
    val = result.GetPointData().GetArray("pointScalars").GetValue(0)
    assert (val > 83.1 and val < 83.2)

    val = result.GetCellData().GetArray("Volume").GetValue(0)
    assert (val > 1.999 and val < 2.01)

def Test3(datadir):
    reader = vtk.vtkDataSetReader()
    reader.SetFileName(datadir + "/Data/blow.vtk")
    reader.UpdateInformation();
    reader.ReadAllScalarsOn()
    reader.ReadAllVectorsOn()

    dssf = vtk.vtkDataSetSurfaceFilter()
    dssf.SetInputConnection(reader.GetOutputPort())

    stripper = vtk.vtkStripper()
    stripper.SetInputConnection(dssf.GetOutputPort())

    f = vtk.vtkIntegrateAttributes()
    f.SetInputConnection(stripper.GetOutputPort())
    f.Update()

    result = f.GetOutputDataObject(0)
    val = result.GetPointData().GetArray("displacement1").GetValue(0)
    assert (val > 463.64 and val < 463.642)

    val = result.GetPointData().GetArray("thickness3").GetValue(0)
    assert (val > 874.61 and val < 874.618)

    val = result.GetCellData().GetArray("Area").GetValue(0)
    assert (val > 1145.405 and val < 1145.415)

Test1(VTK_DATA_ROOT)
Test2(VTK_DATA_ROOT)
Test3(VTK_DATA_ROOT)
