from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkIOLegacy as il
from vtkmodules import vtkIOCellGrid as ic

from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetTempDir

import os

class TestLegacyCellGridWriterReader(Testing.vtkTest):

    def test(self):

        tmpdir = vtkGetTempDir()

        rcg = ic.vtkCellGridReader()
        rcg.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgTriangle.dg'))
        rcg.Update()

        wri = il.vtkLegacyCellGridWriter()
        wri.SetInputConnection(rcg.GetOutputPort())
        wri.SetFileName(tmpdir + '/cellgrid.vtk')
        print(tmpdir + '/cellgrid.vtk')
        wri.Write()

        rdr = il.vtkLegacyCellGridReader()
        rdr.SetFileName(tmpdir + '/cellgrid.vtk')
        rdr.Update()

        cg = rdr.GetOutput()
        print(cg.GetNumberOfCells(), 'cells')
        self.assertEqual(cg.GetNumberOfCells(), 2, 'Expected 2 cells.')

if __name__ == "__main__":
    Testing.main([(TestLegacyCellGridWriterReader, 'test')])
