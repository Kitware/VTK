#!/usr/bin/env python

from vtkmodules.vtkIOFides import vtkFidesWriter, vtkFidesReader
from vtkmodules.vtkCommonDataModel import vtkPartitionedDataSetCollection, vtkPolyData
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.util.misc import vtkGetTempDir
from vtk.util.vtkAlgorithm import VTKPythonAlgorithmBase
try:
    # Not directly used here, but if it's not imported when VTK_USE_MPI, we get following error:
    # Attempting to use an MPI routine before initializing MPICH
    from mpi4py import MPI
except ImportError:
    pass

try:
    import numpy as np
    from vtkmodules.util.numpy_support import numpy_to_vtk
except ImportError:
    print('This test requires numpy!')
    from vtkmodules.test import Testing

    Testing.skip()

import os.path
VTK_TEMP_DIR = vtkGetTempDir()

from vtk.test import Testing

class DataSource(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=0,
                nOutputPorts=1, outputType='vtkPolyData')


    def RequestInformation(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        ts = range(0, 6)
        info.Set(vtkStreamingDemandDrivenPipeline.TIME_STEPS(), ts, len(ts))
        info.Set(vtkStreamingDemandDrivenPipeline.TIME_RANGE(), [ts[0], ts[-1]], 2)
        return 1


    def RequestData(self, request, inInfo, outInfo):
        info = outInfo.GetInformationObject(0)
        output = vtkPolyData.GetData(info)

        timestep = info.Get(vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP())

        sphere = vtkSphereSource()
        sphere.SetRadius(1.0)
        sphere.SetThetaResolution(16)
        sphere.SetPhiResolution(16)
        sphere.SetCenter(timestep * 2.0, 0, 0)
        sphere.Update()

        data = sphere.GetOutput()
        numPts = data.GetNumberOfPoints()
        npArray = np.sin(timestep + np.linspace(0, np.pi, numPts))

        array = numpy_to_vtk(npArray)
        array.SetName('MyPointField')

        data.GetPointData().AddArray(array)
        output.ShallowCopy(data)

        return 1


class TestFidesWriterTime(Testing.vtkTest):
    def testTime(self):
        source = DataSource()
        source.Update()

        # write all time steps
        filename = os.path.join(VTK_TEMP_DIR, 'testTime1.bp')
        writer = vtkFidesWriter()
        writer.SetFileName(filename)
        writer.SetInputConnection(source.GetOutputPort())
        writer.Write()

        reader = vtkFidesReader()
        reader.SetFileName(filename)
        reader.UpdateInformation()
        self.assertTrue(reader.GetOutputInformation(0).Has(
            vtkStreamingDemandDrivenPipeline.TIME_STEPS()))
        nsteps = reader.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
        self.assertEqual(nsteps, 6)

        self.assertEqual(reader.GetNumberOfPointArrays(), 2)
        reader.ConvertToVTKOn()
        reader.Update()

        pdsc = reader.GetOutputDataObject(0)
        self.assertTrue(isinstance(pdsc, vtkPartitionedDataSetCollection))
        pds = pdsc.GetPartitionedDataSet(0)
        nParts = pds.GetNumberOfPartitions()
        self.assertEqual(nParts, 1)

        ds = pds.GetPartition(0)
        self.assertEqual(ds.GetNumberOfCells(), 448)
        self.assertEqual(ds.GetNumberOfPoints(), 226)

        # do a test for writing only some time steps
        writer = vtkFidesWriter()
        filename = os.path.join(VTK_TEMP_DIR, 'testTime2.bp')
        writer.SetFileName(filename)
        writer.SetInputConnection(source.GetOutputPort())
        writer.SetTimeStepStride(2)
        writer.Write()

        reader = vtkFidesReader()
        reader.SetFileName(filename)
        reader.UpdateInformation()
        self.assertTrue(reader.GetOutputInformation(0).Has(
            vtkStreamingDemandDrivenPipeline.TIME_STEPS()))
        nsteps = reader.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
        self.assertEqual(nsteps, 3)

        self.assertEqual(reader.GetNumberOfPointArrays(), 2)
        reader.ConvertToVTKOn()
        reader.Update()

        pdsc = reader.GetOutputDataObject(0)
        self.assertTrue(isinstance(pdsc, vtkPartitionedDataSetCollection))
        pds = pdsc.GetPartitionedDataSet(0)
        nParts = pds.GetNumberOfPartitions()
        self.assertEqual(nParts, 1)

        ds = pds.GetPartition(0)
        self.assertEqual(ds.GetNumberOfCells(), 448)
        self.assertEqual(ds.GetNumberOfPoints(), 226)


if __name__ == "__main__":
    Testing.main([(TestFidesWriterTime, 'test')])
