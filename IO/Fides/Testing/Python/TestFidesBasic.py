#!/usr/bin/env python

from vtkmodules import vtkIOFides
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.util.misc import vtkGetDataRoot
try:
    # Not directly used here, but if it's not imported when VTK_USE_MPI, we get following error:
    # Attempting to use an MPI routine before initializing MPICH
    from mpi4py import MPI
except ImportError:
    pass

VTK_DATA_ROOT = vtkGetDataRoot()

from vtk.test import Testing

class TestFidesBasic(Testing.vtkTest):
    def testFirst(self):
        r = vtkIOFides.vtkFidesReader()
        r.ParseDataModel(VTK_DATA_ROOT + "/Data/vtk-uns-grid-2.json")
        r.SetDataSourcePath("source",
            VTK_DATA_ROOT + "/Data/tris-blocks-time.bp")
        r.UpdateInformation()
        self.assertTrue(r.GetOutputInformation(0).Has(
            vtkStreamingDemandDrivenPipeline.TIME_STEPS()))
        nsteps = r.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
        self.assertEqual(nsteps, 5)
        for i in range(nsteps):
            self.assertEqual(r.GetOutputInformation(0).Get(
                vtkStreamingDemandDrivenPipeline.TIME_STEPS(),i), i)
        pds = r.GetPointDataArraySelection()
        pds.DisableAllArrays()
        pds.EnableArray("dpot2")
        r.ConvertToVTKOn()
        r.Update()

        pds = r.GetOutputDataObject(0)
        nParts = pds.GetNumberOfPartitions()
        self.assertEqual(nParts, 4)

        ds = pds.GetPartition(0)
        self.assertEqual(ds.GetNumberOfCells(), 24)
        self.assertEqual(ds.GetNumberOfPoints(), 20)
        self.assertEqual(ds.GetPointData().GetNumberOfArrays(), 1)
        self.assertEqual(ds.GetPointData().GetArray(0).GetName(), "dpot2")
        return

    def testSecond(self):
        r = vtkIOFides.vtkFidesReader()
        r.ParseDataModel(VTK_DATA_ROOT + "/Data/vtk-uns-grid-2.json")
        r.SetDataSourcePath("source",
            VTK_DATA_ROOT + "/Data/tris-blocks-time.bp")
        r.UpdateInformation()
        self.assertTrue(r.GetOutputInformation(0).Has(
            vtkStreamingDemandDrivenPipeline.TIME_STEPS()))
        nsteps = r.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
        self.assertEqual(nsteps, 5)
        for i in range(nsteps):
            self.assertEqual(r.GetOutputInformation(0).
                             Get(vtkStreamingDemandDrivenPipeline.TIME_STEPS(),i), i)
        pds = r.GetPointDataArraySelection()
        pds.DisableAllArrays()
        pds.EnableArray("dpot2")
        r.Update()

        pds = r.GetOutputDataObject(0)
        nParts = pds.GetNumberOfPartitions()
        self.assertEqual(nParts, 4)

        ds = pds.GetPartition(0)
        self.assertEqual(ds.GetNumberOfCells(), 24)
        self.assertEqual(ds.GetNumberOfPoints(), 20)
        self.assertEqual(ds.GetPointData().GetNumberOfArrays(), 1)
        self.assertEqual(ds.GetPointData().GetArray(0).GetName(), "dpot2")
        self.assertEqual(ds.GetClassName(), "vtkmDataSet")
        return

    def testThird(self):
        r = vtkIOFides.vtkFidesReader()
        r.SetFileName(VTK_DATA_ROOT + "/Data/vtk-uns-grid-2.json")
        r.SetDataSourcePath("source",
            VTK_DATA_ROOT + "/Data/tris-blocks-time.bp")
        r.UpdateInformation()
        r.PrepareNextStep()
        r.Update()
        r.PrepareNextStep()
        r.Update()
        r.PrepareNextStep()
        r.Update()

        pds = r.GetOutputDataObject(0)
        nParts = pds.GetNumberOfPartitions()
        self.assertEqual(nParts, 4)

        ds = pds.GetPartition(0)
        self.assertEqual(ds.GetNumberOfCells(), 24)
        self.assertEqual(ds.GetNumberOfPoints(), 20)
        self.assertEqual(ds.GetPointData().GetNumberOfArrays(), 2)
        self.assertEqual(ds.GetPointData().GetArray(0).GetName(), "dpot")
        self.assertEqual(ds.GetClassName(), "vtkmDataSet")
        return

    def testFourth(self):
        # This test is based on selecting a BP file that contains some attributes that helps Fides
        # generate a data model, instead of the user providing the data model.
        r = vtkIOFides.vtkFidesReader()
        r.SetFileName(VTK_DATA_ROOT + "/Data/cartesian-attr.bp")
        r.UpdateInformation()
        self.assertTrue(r.GetOutputInformation(0).Has(
            vtkStreamingDemandDrivenPipeline.TIME_STEPS()))
        nsteps = r.GetOutputInformation(0).Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
        self.assertEqual(nsteps, 1)
        for i in range(nsteps):
            self.assertEqual(r.GetOutputInformation(0).Get(
                vtkStreamingDemandDrivenPipeline.TIME_STEPS(),i), i)
        pds = r.GetPointDataArraySelection()
        pds.DisableAllArrays()
        pds.EnableArray("density")
        r.ConvertToVTKOn()
        r.Update()

        pds = r.GetOutputDataObject(0)
        nParts = pds.GetNumberOfPartitions()
        self.assertEqual(nParts, 1)

        ds = pds.GetPartition(0)
        self.assertEqual(ds.GetNumberOfCells(), 440)
        self.assertEqual(ds.GetNumberOfPoints(), 648)
        self.assertEqual(ds.GetPointData().GetNumberOfArrays(), 1)
        self.assertEqual(ds.GetPointData().GetArray(0).GetName(), "density")
        return

if __name__ == "__main__":
    Testing.main([(TestFidesBasic, 'test')])
