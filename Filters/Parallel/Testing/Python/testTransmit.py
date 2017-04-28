import vtk
from vtk.util.misc import vtkGetTempDir
VTK_TEMP_DIR = vtkGetTempDir()

contr = vtk.vtkMultiProcessController.GetGlobalController()
if not contr:
    nranks = 1
    rank = 0
else:
    nranks = contr.GetNumberOfProcesses()
    rank = contr.GetLocalProcessId()

def GetSource(dataType):
    s = vtk.vtkRTAnalyticSource()
    # Fake serial source
    if rank == 0:
        s.Update()

    if dataType == 'ImageData':
        return s.GetOutput()

    elif dataType == 'UnstructuredGrid':
        dst = vtk.vtkDataSetTriangleFilter()
        dst.SetInputData(s.GetOutput())
        dst.Update()
        return dst.GetOutput()

    elif dataType == 'RectilinearGrid':

        input = s.GetOutput()

        rg = vtk.vtkRectilinearGrid()
        rg.SetExtent(input.GetExtent())
        dims = input.GetDimensions()
        spacing = input.GetSpacing()

        x = vtk.vtkFloatArray()
        x.SetNumberOfTuples(dims[0])
        for i in range(dims[0]):
            x.SetValue(i, spacing[0]*i)

        y = vtk.vtkFloatArray()
        y.SetNumberOfTuples(dims[1])
        for i in range(dims[1]):
            y.SetValue(i, spacing[1]*i)

        z = vtk.vtkFloatArray()
        z.SetNumberOfTuples(dims[2])
        for i in range(dims[2]):
            z.SetValue(i, spacing[2]*i)

        rg.SetXCoordinates(x)
        rg.SetYCoordinates(y)
        rg.SetZCoordinates(z)

        rg.GetPointData().ShallowCopy(input.GetPointData())

        return rg

    elif dataType == 'StructuredGrid':

        input = s.GetOutput()

        sg = vtk.vtkStructuredGrid()
        sg.SetExtent(input.GetExtent())
        pts = vtk.vtkPoints()
        sg.SetPoints(pts)
        npts = input.GetNumberOfPoints()
        for i in xrange(npts):
            pts.InsertNextPoint(input.GetPoint(i))
        sg.GetPointData().ShallowCopy(input.GetPointData())

        return sg

def TestDataType(dataType, filter):
    if rank == 0:
        print dataType

    s = GetSource(dataType)

    da = vtk.vtkIntArray()
    da.SetNumberOfTuples(6)
    if rank == 0:
        try:
            ext = s.GetExtent()
        except:
            ext = (0, -1, 0, -1, 0, -1)
        for i in range(6):
            da.SetValue(i, ext[i])
    contr.Broadcast(da, 0)

    ext = []
    for i in range(6):
        ext.append(da.GetValue(i))
    ext = tuple(ext)

    tp = vtk.vtkTrivialProducer()
    tp.SetOutput(s)
    tp.SetWholeExtent(ext)

    ncells = vtk.vtkIntArray()
    ncells.SetNumberOfTuples(1)
    if rank == 0:
        ncells.SetValue(0, s.GetNumberOfCells())
    contr.Broadcast(ncells, 0)

    result = vtk.vtkIntArray()
    result.SetNumberOfTuples(1)
    result.SetValue(0, 1)

    if rank > 0:
        if s.GetNumberOfCells() != 0:
            result.SetValue(0, 0)

    filter.SetInputConnection(tp.GetOutputPort())
    filter.Update(rank, nranks, 0)

    if filter.GetOutput().GetNumberOfCells() != ncells.GetValue(0) / nranks:
        result.SetValue(0, 0)

    filter.Update(rank, nranks, 1)

    gl = filter.GetOutput().GetCellData().GetArray(vtk.vtkDataSetAttributes.GhostArrayName())
    if not gl:
        result.SetValue(0, 0)
    else:
        rng = gl.GetRange()
        if rng[1] != 1:
            result.SetValue(0, 0)

    resArray = vtk.vtkIntArray()
    resArray.SetNumberOfTuples(1)
    contr.AllReduce(result, resArray, vtk.vtkCommunicator.MIN_OP)

    assert resArray.GetValue(0) == 1


TestDataType('ImageData', vtk.vtkTransmitImageDataPiece())
TestDataType('RectilinearGrid', vtk.vtkTransmitRectilinearGridPiece())
TestDataType('StructuredGrid', vtk.vtkTransmitStructuredGridPiece())
TestDataType('UnstructuredGrid', vtk.vtkTransmitUnstructuredGridPiece())
