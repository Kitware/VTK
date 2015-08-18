from __future__ import print_function

import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetTempDir
VTK_TEMP_DIR = vtkGetTempDir()

contr = vtk.vtkMultiProcessController.GetGlobalController()
if not contr:
    nranks = 1
    rank = 0
else:
    nranks = contr.GetNumberOfProcesses()
    rank = contr.GetLocalProcessId()

pf = vtk.vtkProgrammableFilter()

def execute():
    info = pf.GetOutputInformation(0)
    et = vtk.vtkExtentTranslator()
    et.SetWholeExtent(info.Get(vtk.vtkStreamingDemandDrivenPipeline.WHOLE_EXTENT()))
    et.SetPiece(info.Get(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_PIECE_NUMBER()))
    et.SetNumberOfPieces(info.Get(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_NUMBER_OF_PIECES()))
    et.PieceToExtent()
    output = pf.GetOutput()
    input = pf.GetInput()
    output.ShallowCopy(input)
    output.Crop(et.GetExtent())

pf.SetExecuteMethod(execute)

def GetSource(dataType):
    s = vtk.vtkRTAnalyticSource()

    if dataType == 'ImageData':
        return s

    elif dataType == 'UnstructuredGrid':
        dst = vtk.vtkDataSetTriangleFilter()
        dst.SetInputConnection(s.GetOutputPort())
        return dst

    elif dataType == 'RectilinearGrid':
        s.Update()

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

        pf.SetInputData(rg)
        return pf

    elif dataType == 'StructuredGrid':
        s.Update()

        input = s.GetOutput()

        sg = vtk.vtkStructuredGrid()
        sg.SetExtent(input.GetExtent())
        pts = vtk.vtkPoints()
        sg.SetPoints(pts)
        npts = input.GetNumberOfPoints()
        for i in range(npts):
            pts.InsertNextPoint(input.GetPoint(i))
        sg.GetPointData().ShallowCopy(input.GetPointData())

        pf.SetInputData(sg)
        return pf

def TestDataType(dataType, reader, writer, ext, numTris):
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
    #writer.SetDataModeToAscii()
    writer.Write()

    if contr:
        contr.Barrier()

    reader.SetFileName(filename)

    cf = vtk.vtkContourFilter()
    cf.SetValue(0, 130)
    cf.SetComputeNormals(0)
    cf.SetComputeGradients(0)
    cf.SetInputConnection(reader.GetOutputPort())
    cf.UpdateInformation()
    cf.GetOutputInformation(0).Set(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_NUMBER_OF_PIECES(), nranks)
    cf.GetOutputInformation(0).Set(vtk.vtkStreamingDemandDrivenPipeline.UPDATE_PIECE_NUMBER(), rank)
    cf.Update()

    ntris = cf.GetOutput().GetNumberOfCells()
    da = vtk.vtkIntArray()
    da.InsertNextValue(ntris)

    da2 = vtk.vtkIntArray()
    da2.SetNumberOfTuples(1)
    contr.AllReduce(da, da2, vtk.vtkCommunicator.SUM_OP)

    if rank == 0:
        print(da2.GetValue(0))
        import os
        os.remove(filename)
        for i in range(npieces):
            os.remove(VTK_TEMP_DIR + "/%s_%d.%s" % (dataType, i, ext))

    assert da2.GetValue(0) == numTris

TestDataType('ImageData', vtk.vtkXMLPImageDataReader(), vtk.vtkXMLPImageDataWriter(), 'vti', 4924)
TestDataType('RectilinearGrid', vtk.vtkXMLPRectilinearGridReader(), vtk.vtkXMLPRectilinearGridWriter(), 'vtr', 4924)
TestDataType('StructuredGrid', vtk.vtkXMLPStructuredGridReader(), vtk.vtkXMLPStructuredGridWriter(), 'vts', 4924)
TestDataType('UnstructuredGrid', vtk.vtkXMLPUnstructuredGridReader(), vtk.vtkXMLPUnstructuredGridWriter(), 'vtu', 11856)
