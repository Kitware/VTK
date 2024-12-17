#!/usr/bin/env python

from vtkmodules.vtkIOFides import vtkFidesWriter, vtkFidesReader
from vtkmodules.vtkCommonDataModel import vtkPartitionedDataSetCollection
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.vtkFiltersCore import vtkConvertToMultiBlockDataSet, vtkConvertToPartitionedDataSetCollection
from vtkmodules.vtkFiltersParallelDIY2 import vtkRedistributeDataSetFilter
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.util.misc import vtkGetTempDir
try:
    # Not directly used here, but if it's not imported when VTK_USE_MPI, we get following error:
    # Attempting to use an MPI routine before initializing MPICH
    from mpi4py import MPI
except ImportError:
    pass

import os.path
VTK_TEMP_DIR = vtkGetTempDir()

from vtk.test import Testing

class TestFidesWriterBasic(Testing.vtkTest):
    def testDataSet(self):
        wavelet = vtkRTAnalyticSource()

        filename = os.path.join(VTK_TEMP_DIR, 'testDataSet.bp')
        writer = vtkFidesWriter()
        writer.SetFileName(filename)
        writer.SetInputConnection(wavelet.GetOutputPort())
        # setting to true but not turning on an array means no array will be written
        writer.SetChooseFieldsToWrite(True)
        writer.Write()

        reader = vtkFidesReader()
        reader.SetFileName(filename)
        reader.UpdateInformation()
        self.assertTrue(reader.GetOutputInformation(0).Has(
            vtkStreamingDemandDrivenPipeline.TIME_STEPS()))
        nsteps = reader.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
        self.assertEqual(nsteps, 1)

        self.assertEqual(reader.GetNumberOfPointArrays(), 0)
        reader.ConvertToVTKOn()
        reader.Update()

        pdsc = reader.GetOutputDataObject(0)
        self.assertTrue(isinstance(pdsc, vtkPartitionedDataSetCollection))
        pds = pdsc.GetPartitionedDataSet(0)
        nParts = pds.GetNumberOfPartitions()
        self.assertEqual(nParts, 1)

        ds = pds.GetPartition(0)
        self.assertEqual(ds.GetNumberOfCells(), 8000)
        self.assertEqual(ds.GetNumberOfPoints(), 9261)


    def testPartitionedDataSet(self):
        wavelet = vtkRTAnalyticSource()
        # get the data into a vtkPartitionedDataSet
        rdsf = vtkRedistributeDataSetFilter()
        rdsf.SetInputConnection(wavelet.GetOutputPort())
        rdsf.SetNumberOfPartitions(4)

        filename = os.path.join(VTK_TEMP_DIR, 'testPDS.bp')
        writer = vtkFidesWriter()
        writer.SetFileName(filename)
        writer.SetInputConnection(rdsf.GetOutputPort())
        writer.Write()

        reader = vtkFidesReader()
        reader.SetFileName(filename)
        reader.UpdateInformation()
        self.assertTrue(reader.GetOutputInformation(0).Has(
            vtkStreamingDemandDrivenPipeline.TIME_STEPS()))
        nsteps = reader.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
        self.assertEqual(nsteps, 1)

        self.assertEqual(reader.GetNumberOfPointArrays(), 1)
        reader.ConvertToVTKOn()
        reader.Update()

        pdsc = reader.GetOutputDataObject(0)
        self.assertTrue(isinstance(pdsc, vtkPartitionedDataSetCollection))
        pds = pdsc.GetPartitionedDataSet(0)
        nParts = pds.GetNumberOfPartitions()
        self.assertEqual(nParts, 1)

        ds = pds.GetPartition(0)
        self.assertEqual(ds.GetNumberOfCells(), 8000)
        self.assertEqual(ds.GetNumberOfPoints(), 9261)


    def testPartitionedDataSetCollection(self):
        wavelet = vtkRTAnalyticSource()

        pdsc = vtkConvertToPartitionedDataSetCollection()
        pdsc.SetInputConnection(wavelet.GetOutputPort())

        filename = os.path.join(VTK_TEMP_DIR, 'testPDSC.bp')
        writer = vtkFidesWriter()
        writer.SetFileName(filename)
        writer.SetInputConnection(pdsc.GetOutputPort())
        writer.Write()

        reader = vtkFidesReader()
        reader.SetFileName(filename)
        reader.UpdateInformation()
        self.assertTrue(reader.GetOutputInformation(0).Has(
            vtkStreamingDemandDrivenPipeline.TIME_STEPS()))
        nsteps = reader.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
        self.assertEqual(nsteps, 1)

        self.assertEqual(reader.GetNumberOfPointArrays(), 1)
        reader.ConvertToVTKOn()
        reader.Update()

        pdsc = reader.GetOutputDataObject(0)
        self.assertTrue(isinstance(pdsc, vtkPartitionedDataSetCollection))
        pds = pdsc.GetPartitionedDataSet(0)
        nParts = pds.GetNumberOfPartitions()
        self.assertEqual(nParts, 1)

        ds = pds.GetPartition(0)
        self.assertEqual(ds.GetNumberOfCells(), 8000)
        self.assertEqual(ds.GetNumberOfPoints(), 9261)


    def testMultiBlockDataSet(self):
        wavelet = vtkRTAnalyticSource()

        mbds = vtkConvertToMultiBlockDataSet()
        mbds.SetInputConnection(wavelet.GetOutputPort())

        filename = os.path.join(VTK_TEMP_DIR, 'testMB.bp')
        writer = vtkFidesWriter()
        writer.SetFileName(filename)
        writer.SetInputConnection(mbds.GetOutputPort())
        writer.Write()

        reader = vtkFidesReader()
        reader.SetFileName(filename)
        reader.UpdateInformation()
        self.assertTrue(reader.GetOutputInformation(0).Has(
            vtkStreamingDemandDrivenPipeline.TIME_STEPS()))
        nsteps = reader.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
        self.assertEqual(nsteps, 1)

        self.assertEqual(reader.GetNumberOfPointArrays(), 1)
        reader.ConvertToVTKOn()
        reader.Update()

        pdsc = reader.GetOutputDataObject(0)
        # vtkFidesReader always reads in a PDSC, as well as vtkFidesWriter always converts MB to PDSC
        self.assertTrue(isinstance(pdsc, vtkPartitionedDataSetCollection))
        pds = pdsc.GetPartitionedDataSet(0)
        nParts = pds.GetNumberOfPartitions()
        self.assertEqual(nParts, 1)

        ds = pds.GetPartition(0)
        self.assertEqual(ds.GetNumberOfCells(), 8000)
        self.assertEqual(ds.GetNumberOfPoints(), 9261)


if __name__ == "__main__":
    Testing.main([(TestFidesWriterBasic, 'test')])
