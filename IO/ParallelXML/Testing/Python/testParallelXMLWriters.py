from __future__ import print_function

from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkIntArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkRectilinearGrid,
    vtkStructuredGrid,
    vtkTable,
)
from vtkmodules.vtkCommonExecutionModel import (
    vtkExtentTranslator,
    vtkStreamingDemandDrivenPipeline,
)
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkFiltersGeneral import vtkDataSetTriangleFilter
from vtkmodules.vtkFiltersProgrammable import vtkProgrammableFilter
from vtkmodules.vtkIOParallelXML import (
    vtkXMLPImageDataWriter,
    vtkXMLPRectilinearGridWriter,
    vtkXMLPStructuredGridWriter,
    vtkXMLPTableWriter,
    vtkXMLPUnstructuredGridWriter,
)
from vtkmodules.vtkIOXML import (
    vtkXMLPImageDataReader,
    vtkXMLPRectilinearGridReader,
    vtkXMLPStructuredGridReader,
    vtkXMLPTableReader,
    vtkXMLPUnstructuredGridReader,
)
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkParallelCore import (
    vtkCommunicator,
    vtkMultiProcessController,
)
from vtkmodules.util.misc import vtkGetTempDir
VTK_TEMP_DIR = vtkGetTempDir()

contr = vtkMultiProcessController.GetGlobalController()
if not contr:
    nranks = 1
    rank = 0
else:
    nranks = contr.GetNumberOfProcesses()
    rank = contr.GetLocalProcessId()

pf = vtkProgrammableFilter()

def execute():
    info = pf.GetOutputInformation(0)
    et = vtkExtentTranslator()

    if pf.GetInput().IsA("vtkDataSet"):
        et.SetWholeExtent(info.Get(vtkStreamingDemandDrivenPipeline.WHOLE_EXTENT()))
        et.SetPiece(info.Get(vtkStreamingDemandDrivenPipeline.UPDATE_PIECE_NUMBER()))
        et.SetNumberOfPieces(info.Get(vtkStreamingDemandDrivenPipeline.UPDATE_NUMBER_OF_PIECES()))
        et.PieceToExtent()

    output = pf.GetOutput()
    input = pf.GetInput()
    output.ShallowCopy(input)
    if pf.GetInput().IsA("vtkDataSet"):
        output.Crop(et.GetExtent())

pf.SetExecuteMethod(execute)

def GetSource(dataType):
    s = vtkRTAnalyticSource()

    if dataType == 'ImageData':
        return s

    elif dataType == 'UnstructuredGrid':
        dst = vtkDataSetTriangleFilter()
        dst.SetInputConnection(s.GetOutputPort())
        return dst

    elif dataType == 'RectilinearGrid':
        s.Update()

        input = s.GetOutput()

        rg = vtkRectilinearGrid()
        rg.SetExtent(input.GetExtent())
        dims = input.GetDimensions()
        spacing = input.GetSpacing()

        x = vtkFloatArray()
        x.SetNumberOfTuples(dims[0])
        for i in range(dims[0]):
            x.SetValue(i, spacing[0]*i)

        y = vtkFloatArray()
        y.SetNumberOfTuples(dims[1])
        for i in range(dims[1]):
            y.SetValue(i, spacing[1]*i)

        z = vtkFloatArray()
        z.SetNumberOfTuples(dims[2])
        for i in range(dims[2]):
            z.SetValue(i, spacing[2]*i)

        rg.SetXCoordinates(x)
        rg.SetYCoordinates(y)
        rg.SetZCoordinates(z)

        rg.GetPointData().ShallowCopy(input.GetPointData())

        pf.SetInputData(rg)
        return pf

    elif dataType == 'StructuredGrid':
        s.Update()

        input = s.GetOutput()

        sg = vtkStructuredGrid()
        sg.SetExtent(input.GetExtent())
        pts = vtkPoints()
        sg.SetPoints(pts)
        npts = input.GetNumberOfPoints()
        for i in range(npts):
            pts.InsertNextPoint(input.GetPoint(i))
        sg.GetPointData().ShallowCopy(input.GetPointData())

        pf.SetInputData(sg)
        return pf

    elif dataType == 'Table':
        s.Update()
        input = s.GetOutput()

        table = vtkTable()
        RTData = input.GetPointData().GetArray(0)
        nbTuples = RTData.GetNumberOfTuples()

        array = vtkFloatArray()
        array.SetName("RTData")
        array.SetNumberOfTuples(nbTuples)

        for i in range(0, nbTuples):
            array.SetTuple1(i, float(RTData.GetTuple1(i)))

        table.AddColumn(array)

        pf.SetInputData(table)
        return pf

def TestDataType(dataType, reader, writer, ext, numTris, useSubdir=False):
    s = GetSource(dataType)

    filename = VTK_TEMP_DIR + "/%s.p%s" % (dataType, ext)

    writer.SetInputConnection(s.GetOutputPort())
    npieces = 16
    writer.SetNumberOfPieces(npieces)
    pperrank = npieces // nranks
    start = pperrank * rank
    end = start + pperrank - 1
    writer.SetStartPiece(start)
    writer.SetEndPiece(end)
    writer.SetFileName(filename)
    if useSubdir:
        writer.SetUseSubdirectory(True)
    #writer.SetDataModeToAscii()
    writer.Write()

    if contr:
        contr.Barrier()

    reader.SetFileName(filename)

    ntris = 0

    if dataType != "Table":
        cf = vtkContourFilter()
        cf.SetValue(0, 130)
        cf.SetComputeNormals(0)
        cf.SetComputeGradients(0)
        cf.SetInputConnection(reader.GetOutputPort())
        cf.UpdateInformation()
        cf.GetOutputInformation(0).Set(vtkStreamingDemandDrivenPipeline.UPDATE_NUMBER_OF_PIECES(), nranks)
        cf.GetOutputInformation(0).Set(vtkStreamingDemandDrivenPipeline.UPDATE_PIECE_NUMBER(), rank)
        cf.Update()

        ntris = cf.GetOutput().GetNumberOfCells()
    else:
        reader.UpdateInformation()
        reader.GetOutputInformation(0).Set(vtkStreamingDemandDrivenPipeline.UPDATE_NUMBER_OF_PIECES(), nranks)
        reader.GetOutputInformation(0).Set(vtkStreamingDemandDrivenPipeline.UPDATE_PIECE_NUMBER(), rank)
        reader.Update()
        ntris = reader.GetOutput().GetNumberOfRows()

    da = vtkIntArray()
    da.InsertNextValue(ntris)

    da2 = vtkIntArray()
    da2.SetNumberOfTuples(1)
    contr.AllReduce(da, da2, vtkCommunicator.SUM_OP)

    if rank == 0:
        print(da2.GetValue(0))
        import os
        os.remove(filename)
        for i in range(npieces):
            if not useSubdir:
                os.remove(VTK_TEMP_DIR + "/%s_%d.%s" % (dataType, i, ext))
            else:
                os.remove(VTK_TEMP_DIR + "/%s/%s_%d.%s" %(dataType, dataType, i, ext))

    assert da2.GetValue(0) == numTris

TestDataType('ImageData', vtkXMLPImageDataReader(), vtkXMLPImageDataWriter(), 'vti', 4924)
TestDataType('RectilinearGrid', vtkXMLPRectilinearGridReader(), vtkXMLPRectilinearGridWriter(), 'vtr', 4924)
TestDataType('StructuredGrid', vtkXMLPStructuredGridReader(), vtkXMLPStructuredGridWriter(), 'vts', 4924)
TestDataType('UnstructuredGrid', vtkXMLPUnstructuredGridReader(), vtkXMLPUnstructuredGridWriter(), 'vtu', 11856)
TestDataType('Table', vtkXMLPTableReader(), vtkXMLPTableWriter(), 'vtt', 18522)

# Test writers with UseSubdirectory on
TestDataType('ImageData', vtkXMLPImageDataReader(), vtkXMLPImageDataWriter(), 'vti', 4924, useSubdir=True)
TestDataType('RectilinearGrid', vtkXMLPRectilinearGridReader(), vtkXMLPRectilinearGridWriter(), 'vtr', 4924, useSubdir=True)
TestDataType('StructuredGrid', vtkXMLPStructuredGridReader(), vtkXMLPStructuredGridWriter(), 'vts', 4924, useSubdir=True)
TestDataType('UnstructuredGrid', vtkXMLPUnstructuredGridReader(), vtkXMLPUnstructuredGridWriter(), 'vtu', 11856, useSubdir=True)
TestDataType('Table', vtkXMLPTableReader(), vtkXMLPTableWriter(), 'vtt', 18522, useSubdir=True)
