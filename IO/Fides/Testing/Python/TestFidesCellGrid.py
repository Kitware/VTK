#!/usr/bin/env python

import os.path
from vtkmodules.vtkIOFides import vtkFidesReader, vtkFidesWriter
from vtkmodules.vtkCommonDataModel import vtkPartitionedDataSetCollection, vtkCellGrid
from vtkmodules.util.misc import vtkGetDataRoot, vtkGetTempDir
try:
    # Not directly used here, but if it's not imported when VTK_USE_MPI, we get following error:
    # Attempting to use an MPI routine before initializing MPICH
    from mpi4py import MPI
except ImportError:
    pass

VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()

from vtk.test import Testing


def read_source():
    r = vtkFidesReader()
    r.ParseDataModel(VTK_DATA_ROOT + "/Data/vtk-cellgrid.json")
    r.SetDataSourcePath("source", VTK_DATA_ROOT + "/Data/dgHexahedra.bp")
    r.Update()
    return r.GetOutputDataObject(0)


def attribute_summary(cg):
    """List of (name, space, num_components) tuples, sorted, for stable compare."""
    ids = list(cg.GetUnorderedCellAttributeIds())
    triples = []
    for aid in ids:
        a = cg.GetCellAttributeById(aid)
        triples.append((a.GetName().Data(), a.GetSpace().Data(), a.GetNumberOfComponents()))
    return sorted(triples)


class TestFidesCellGrid(Testing.vtkTest):
    def _assertCellGridEquivalent(self, original_cg, rt_cg):
        self.assertIsInstance(original_cg, vtkCellGrid)
        self.assertIsInstance(rt_cg, vtkCellGrid)
        self.assertEqual(rt_cg.GetNumberOfCells(), original_cg.GetNumberOfCells())
        self.assertEqual(attribute_summary(rt_cg), attribute_summary(original_cg))

    def testRoundTripPDSC(self):
        # The reader produces a vtkPartitionedDataSetCollection holding the
        # cellgrid as a partition. Hand that PDSC straight to the writer.
        original_pdsc = read_source()
        self.assertIsInstance(original_pdsc, vtkPartitionedDataSetCollection)
        original_cg = original_pdsc.GetPartitionedDataSet(0).GetPartitionAsDataObject(0)
        self.assertIsInstance(original_cg, vtkCellGrid)

        out_bp = os.path.join(VTK_TEMP_DIR, "TestFidesCellGrid_pdsc.bp")
        writer = vtkFidesWriter()
        writer.SetFileName(out_bp)
        writer.SetInputData(original_pdsc)
        writer.Write()

        reader = vtkFidesReader()
        reader.SetFileName(out_bp)
        reader.Update()
        rt_pdsc = reader.GetOutputDataObject(0)
        self.assertIsInstance(rt_pdsc, vtkPartitionedDataSetCollection)
        rt_cg = rt_pdsc.GetPartitionedDataSet(0).GetPartitionAsDataObject(0)
        self._assertCellGridEquivalent(original_cg, rt_cg)

    def testRoundTripBareCellGrid(self):
        # Pass a bare vtkCellGrid (not wrapped in a PDSC) directly to the
        # writer. vtkCellGrid isn't a vtkDataSet so the writer needs an
        # explicit accept + auto-wrap for this case.
        original_pdsc = read_source()
        original_cg = original_pdsc.GetPartitionedDataSet(0).GetPartitionAsDataObject(0)
        self.assertIsInstance(original_cg, vtkCellGrid)

        out_bp = os.path.join(VTK_TEMP_DIR, "TestFidesCellGrid_bare.bp")
        writer = vtkFidesWriter()
        writer.SetFileName(out_bp)
        writer.SetInputData(original_cg)
        writer.Write()

        reader = vtkFidesReader()
        reader.SetFileName(out_bp)
        reader.Update()
        rt_pdsc = reader.GetOutputDataObject(0)
        self.assertIsInstance(rt_pdsc, vtkPartitionedDataSetCollection)
        rt_cg = rt_pdsc.GetPartitionedDataSet(0).GetPartitionAsDataObject(0)
        self._assertCellGridEquivalent(original_cg, rt_cg)


if __name__ == "__main__":
    Testing.main([(TestFidesCellGrid, 'test')])
