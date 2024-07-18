from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkIOLegacy as il
from vtkmodules import vtkIOCellGrid as ic

from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetTempDir

import os

class TestLegacyCompositeCellGridWriterReader(Testing.vtkTest):

    def test(self):

        tmpdir = vtkGetTempDir()

        rc1 = ic.vtkCellGridReader()
        rc1.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgTriangle.dg'))
        rc1.Update()

        rc2 = ic.vtkCellGridReader()
        rc2.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgQuadrilateral.dg'))
        rc2.Update()

        cg1 = rc1.GetOutput()
        cg2 = rc2.GetOutput()
        pdc = dm.vtkPartitionedDataSetCollection()
        pdc.SetNumberOfPartitionedDataSets(2)
        pdc.SetNumberOfPartitions(0, 1)
        pdc.SetNumberOfPartitions(1, 1)
        pdc.SetPartition(0, 0, cg1)
        pdc.SetPartition(1, 0, cg2)

        wri = il.vtkGenericDataObjectWriter()
        wri.SetInputDataObject(pdc)
        wri.SetFileName(tmpdir + '/cellgrid-composite.vtk')
        wri.Write()

        rdr = il.vtkGenericDataObjectReader()
        rdr.SetFileName(tmpdir + '/cellgrid-composite.vtk')
        rdr.Update()

        cg = rdr.GetOutput()
        print(cg.GetNumberOfCells(), 'cells')
        self.assertEqual(cg.GetNumberOfCells(), 4, 'Expected 4 cells.')

if __name__ == "__main__":
    Testing.main([(TestLegacyCompositeCellGridWriterReader, 'test')])
