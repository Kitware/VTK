from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtkmodules import vtkIOCellGrid as io
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

from vtkmodules.test import Testing
import os

class TestCellGrid(Testing.vtkTest):

    def test(self):
        print('foo')

    def testConstruction(self):
        g = dm.vtkCellGrid()
        self.assertEqual(g.GetNumberOfCells(), 0)
        self.assertEqual(g.GetShapeAttribute(), None)
        bds = [0, 0, 0, 0, 0, 0]
        g.GetBounds(bds)
        self.assertEqual(bds, [1., -1., 1., -1., 1., -1.])

    def testReader(self):
        r = io.vtkCellGridReader()
        r.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg'))
        r.Update()
        g = r.GetOutput()
        self.assertEqual(g.GetNumberOfCells(), 2)
        s = g.GetShapeAttribute()
        self.assertEqual(s.GetNumberOfComponents(), 3)
        bds = [0, 0, 0, 0, 0, 0]
        g.GetBounds(bds)
        self.assertEqual(bds, [0.0, 2.0, 0.0, 1.25, 0.0, 1.0])
        # TODO: Fetch cell types, cell metadata

if __name__ == "__main__":
    Testing.main([(TestCellGrid, 'test')])
