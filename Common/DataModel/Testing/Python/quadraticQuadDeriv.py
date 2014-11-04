#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

from vtk.test import Testing

class TestCommand(Testing.vtkTest):
    def _test(self, fname):
        reader = vtk.vtkXMLUnstructuredGridReader()
        reader.SetFileName(VTK_DATA_ROOT + fname)

        elev = vtk.vtkElevationFilter()
        elev.SetInputConnection(reader.GetOutputPort())
        elev.SetLowPoint(-0.05, 0.05, 0)
        elev.SetHighPoint(0.05, 0.05, 0)

        grad = vtk.vtkGradientFilter()
        grad.SetInputConnection(elev.GetOutputPort())
        grad.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_POINTS, "Elevation")

        grad.Update()

        vals = (10, 0, 0)

        for i in range(3):
            r = grad.GetOutput().GetPointData().GetArray("Gradients").GetRange(i)

            self.assertTrue(abs(r[0] - vals[i]) < 1E-4)
            self.assertTrue(abs(r[1] - vals[i]) < 1E-4)

        elev.SetLowPoint(0.05, -0.05, 0)
        elev.SetHighPoint(0.05, 0.05, 0)
        grad.Update()

        vals = (0, 10, 0)

        for i in range(3):
            r = grad.GetOutput().GetPointData().GetArray("Gradients").GetRange(i)

            self.assertTrue(abs(r[0] - vals[i]) < 1E-4)
            self.assertTrue(abs(r[1] - vals[i]) < 1E-4)

    def testQuadraticQuad(self):
        self._test("/Data/Disc_QuadraticQuads_0_0.vtu")

    def testBiQuadraticQuad(self):
        self._test("/Data/Disc_BiQuadraticQuads_0_0.vtu")

if __name__ == "__main__":
    Testing.main([(TestCommand, 'test')])
