import sys
import vtk
from vtk.test import Testing

from vtk.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

try:
    import numpy
except ImportError:
    print("WARNING: This test requires Numeric Python: http://numpy.sf.net")
    from vtk.test import Testing
    Testing.skip()

from vtk.util import numpy_support as np_s

def makePolyData(attributeType):
    ss = vtk.vtkSphereSource()
    ss.Update()
    s = ss.GetOutput()

    pd = s.GetAttributes(attributeType)

    num = 0
    if attributeType == vtk.vtkDataObject.POINT:
        num = s.GetNumberOfPoints()
    else:
        num = s.GetNumberOfCells()

    a = [ x for x in range(num)]
    aa = numpy.array(a)

    b = [ x**2 for x in a]
    bb = numpy.array(b)

    array1 = np_s.numpy_to_vtk(aa, deep=True)
    array1.SetName("ID")

    array2 = np_s.numpy_to_vtk(bb, deep=True)
    array2.SetName("Square")

    pd.AddArray(array1)
    pd.AddArray(array2)

    return s


def makeGraph(attributeType):
    reader = vtk.vtkPDBReader()
    reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/3GQP.pdb")
    reader.Update()

    molecule = reader.GetOutputDataObject(1)

    pd = molecule.GetAttributes(attributeType)

    num = 0
    if attributeType == vtk.vtkDataObject.VERTEX:
        num = molecule.GetNumberOfVertices()
    else:
        num = molecule.GetNumberOfEdges()

    a = [ x for x in range(num)]
    aa = numpy.array(a)

    b = [ x**2 for x in a]
    bb = numpy.array(b)

    array1 = np_s.numpy_to_vtk(aa, deep=True)
    array1.SetName("ID")

    array2 = np_s.numpy_to_vtk(bb, deep=True)
    array2.SetName("Square")

    pd.AddArray(array1)
    pd.AddArray(array2)

    return molecule

def makeTable():
    table = vtk.vtkTable()

    num = 100

    a = [ x for x in range(num)]
    aa = numpy.array(a)

    b = [ x**2 for x in a]
    bb = numpy.array(b)

    array1 = np_s.numpy_to_vtk(aa, deep=True)
    array1.SetName("ID")

    array2 = np_s.numpy_to_vtk(bb, deep=True)
    array2.SetName("Square")

    table.AddColumn(array1)
    table.AddColumn(array2)

    return table

def testCalculator(dataset, attributeType, attributeTypeForCalc=None):
    calc = vtk.vtkArrayCalculator()
    calc.SetInputData(dataset)
    if attributeTypeForCalc is not None:
        calc.SetAttributeType(attributeTypeForCalc)
    calc.AddScalarArrayName("ID")
    calc.AddScalarArrayName("Square")
    calc.SetFunction("ID + Square / 24")
    calc.SetResultArrayName("Result")
    calc.Update()

    fd = calc.GetOutput().GetAttributes(attributeType)
    if not fd.HasArray("Result"):
        print("ERROR: calculator did not produce result array when processing datatype %d" % (attributeType,))


if __name__ == '__main__':
    testCalculator(makePolyData(vtk.vtkDataObject.POINT), vtk.vtkDataObject.POINT)
    testCalculator(makePolyData(vtk.vtkDataObject.CELL), vtk.vtkDataObject.CELL, vtk.vtkDataObject.CELL)
    testCalculator(makeGraph(vtk.vtkDataObject.VERTEX), vtk.vtkDataObject.VERTEX)
    testCalculator(makeGraph(vtk.vtkDataObject.EDGE), vtk.vtkDataObject.EDGE, vtk.vtkDataObject.EDGE)
    testCalculator(makeTable(), vtk.vtkDataObject.ROW)
