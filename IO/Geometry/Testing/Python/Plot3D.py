#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class TestPlot3D(Testing.vtkTest):
    def testReader3D(self):
        names = [("multi-ascii.xyz", "multi-ascii.q"), ("multi-bin-C.xyz", "multi-bin-C.q"), ("multi-bin.xyz", "multi-bin.q"), ("multi-bin.xyz", "multi-bin-oflow.q")]

        for name in names:
            r = vtk.vtkMultiBlockPLOT3DReader()
            print("Testing ", name[0], name[1])
            if name[0] == "multi-ascii.xyz":
                r.BinaryFileOff()
                r.MultiGridOn()
            else:
                r.AutoDetectFormatOn()
            r.SetFileName(str(VTK_DATA_ROOT) + "/Data/" + name[0])
            r.SetQFileName(str(VTK_DATA_ROOT) + "/Data/" + name[1])
            r.Update()

            output = r.GetOutput()
            self.assertEqual(output.GetNumberOfBlocks(), 2)

            b0 = output.GetBlock(0)
            self.assertEqual(int(b0.GetFieldData().GetArray("Properties").GetValue(0)), 2)
            pd = b0.GetPointData()
            self.assertEqual(int(pd.GetArray("Momentum").GetComponent(10, 2)), 0)
            self.assertEqual(int(pd.GetArray("StagnationEnergy").GetValue(3)), 9)

            b1 = output.GetBlock(1)
            self.assertEqual(int(b1.GetFieldData().GetArray("Properties").GetValue(0)), 2)
            pd = b1.GetPointData()
            self.assertEqual(int(pd.GetArray("Momentum").GetComponent(10, 2)), 0)
            self.assertEqual(int(pd.GetArray("StagnationEnergy").GetValue(3)), 3)

    def testReader2D(self):
        names = [("multi-bin-2D.xyz", "multi-bin-2D.q")]

        for name in names:
            r = vtk.vtkMultiBlockPLOT3DReader()
            print("Testing ", name[0])
            r.AutoDetectFormatOn()
            r.SetFileName(str(VTK_DATA_ROOT) + "/Data/" + name[0])
            r.SetQFileName(str(VTK_DATA_ROOT) + "/Data/" + name[1])
            r.Update()

            output = r.GetOutput()
            self.assertEqual(output.GetNumberOfBlocks(), 2)

            b0 = output.GetBlock(0)
            self.assertEqual(int(b0.GetFieldData().GetArray("Properties").GetValue(0)), 2)
            pd = b0.GetPointData()
            self.assertEqual(int(pd.GetArray("Momentum").GetComponent(10, 2)), 0)
            self.assertEqual(int(pd.GetArray("StagnationEnergy").GetValue(3)), 9)

            b1 = output.GetBlock(1)
            self.assertEqual(int(b1.GetFieldData().GetArray("Properties").GetValue(0)), 2)
            pd = b1.GetPointData()
            self.assertEqual(int(pd.GetArray("Momentum").GetComponent(10, 2)), 0)
            self.assertEqual(int(pd.GetArray("StagnationEnergy").GetValue(3)), 1)

    def testMetaReader(self):
        r = vtk.vtkPlot3DMetaReader()
        r.SetFileName(str(VTK_DATA_ROOT) + "/Data/multi.p3d")
        r.Update()
        self.assertTrue(r.GetOutput().GetBlock(0).GetPointData().GetArray("Function0"))

if __name__ == "__main__":
    Testing.main([(TestPlot3D, 'test')])
