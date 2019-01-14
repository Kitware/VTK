#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()
import sys

class TestClip(Testing.vtkTest):
    def testImage2DScalar(self):
        planes = ['XY', 'XZ', 'YZ']
        expectedNCells = [38, 46, 42]
        expectedNClippedCells = [104, 104, 106]
        for plane, nCells, nClippedCells in zip(planes,expectedNCells,expectedNClippedCells):
            r = vtk.vtkRTAnalyticSource()
            r.SetXFreq(600);
            r.SetYFreq(400);
            r.SetZFreq(900);
            if plane == 'XY':
                r.SetWholeExtent(-5, 5, -5, 5, 0, 0)
            elif plane == 'XZ':
                r.SetWholeExtent(-5, 5, 0, 0, -5, 5)
            else:
                r.SetWholeExtent(0, 0, -5, 5, -5, 5)
            r.Update()

            c = vtk.vtkTableBasedClipDataSet()
            c.SetInputConnection(r.GetOutputPort())
            c.SetUseValueAsOffset(0)
            c.SetValue(150)
            c.SetInsideOut(1)
            c.SetGenerateClippedOutput(1)

            c.Update()

            self.assertEqual(c.GetOutput().GetNumberOfCells(), nCells)
            self.assertEqual(c.GetClippedOutput().GetNumberOfCells(), nClippedCells)

    def testImage(self):
        r = vtk.vtkRTAnalyticSource()
        r.SetWholeExtent(-5, 5, -5, 5, -5, 5)
        r.Update()

        s = vtk.vtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = vtk.vtkTableBasedClipDataSet()
        c.SetInputConnection(r.GetOutputPort())
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

    def testRectilinear(self):
        rt = vtk.vtkRTAnalyticSource()
        rt.SetWholeExtent(-5, 5, -5, 5, -5, 5)
        rt.Update()
        i = rt.GetOutput()

        r = vtk.vtkRectilinearGrid()
        dims = i.GetDimensions()
        r.SetDimensions(dims)
        exts = i.GetExtent()
        orgs = i.GetOrigin()

        xs = vtk.vtkFloatArray()
        xs.SetNumberOfTuples(dims[0])
        for d in range(dims[0]):
            xs.SetTuple1(d, orgs[0] + exts[0] + d)
        r.SetXCoordinates(xs)

        ys = vtk.vtkFloatArray()
        ys.SetNumberOfTuples(dims[1])
        for d in range(dims[1]):
            ys.SetTuple1(d, orgs[1] + exts[2] + d)
        r.SetYCoordinates(ys)

        zs = vtk.vtkFloatArray()
        zs.SetNumberOfTuples(dims[2])
        for d in range(dims[2]):
            zs.SetTuple1(d, orgs[2] + exts[4] + d)
        r.SetZCoordinates(zs)

        s = vtk.vtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = vtk.vtkTableBasedClipDataSet()
        c.SetInputData(r)
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

    def testStructured2D(self):
        planes = ['XY', 'XZ', 'YZ']
        expectedNCells = [42, 34, 68]
        for plane, nCells in zip(planes,expectedNCells):
            rt = vtk.vtkRTAnalyticSource()
            if plane == 'XY':
                rt.SetWholeExtent(-5, 5, -5, 5, 0, 0)
            elif plane == 'XZ':
                rt.SetWholeExtent(-5, 5, 0, 0, -5, 5)
            else:
                rt.SetWholeExtent(0, 0, -5, 5, -5, 5)
            rt.Update()
            i = rt.GetOutput()

            st = vtk.vtkStructuredGrid()
            st.SetDimensions(i.GetDimensions())

            nps = i.GetNumberOfPoints()
            ps = vtk.vtkPoints()
            ps.SetNumberOfPoints(nps)
            for idx in range(nps):
                ps.SetPoint(idx, i.GetPoint(idx))

            st.SetPoints(ps)

            cyl = vtk.vtkCylinder()
            cyl.SetRadius(2)
            cyl.SetCenter(0,0,0)
            transform = vtk.vtkTransform()
            transform.RotateWXYZ(45,20,1,10)
            cyl.SetTransform(transform)

            c = vtk.vtkTableBasedClipDataSet()
            c.SetInputData(st)
            c.SetClipFunction(cyl)
            c.SetInsideOut(1)

            c.Update()

            self.assertEqual(c.GetOutput().GetNumberOfCells(), nCells)

    def testStructured(self):
        rt = vtk.vtkRTAnalyticSource()
        rt.SetWholeExtent(-5, 5, -5, 5, -5, 5)
        rt.Update()
        i = rt.GetOutput()

        st = vtk.vtkStructuredGrid()
        st.SetDimensions(i.GetDimensions())

        nps = i.GetNumberOfPoints()
        ps = vtk.vtkPoints()
        ps.SetNumberOfPoints(nps)
        for idx in range(nps):
            ps.SetPoint(idx, i.GetPoint(idx))

        st.SetPoints(ps)

        s = vtk.vtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = vtk.vtkTableBasedClipDataSet()
        c.SetInputData(st)
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

    def testUnstructured(self):
        rt = vtk.vtkRTAnalyticSource()
        rt.SetWholeExtent(-5, 5, -5, 5, -5, 5)

        t = vtk.vtkThreshold()
        t.SetInputConnection(rt.GetOutputPort())
        t.ThresholdByUpper(-10)

        s = vtk.vtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = vtk.vtkTableBasedClipDataSet()
        c.SetInputConnection(t.GetOutputPort())
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

        eg = vtk.vtkEnSightGoldReader()
        eg.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/elements.case")
        eg.Update()

        pl = vtk.vtkPlane()
        pl.SetOrigin(3.5, 3.5, 0.5)
        pl.SetNormal(0, 0, 1)

        c.SetInputConnection(eg.GetOutputPort())
        c.SetClipFunction(pl)
        c.SetInsideOut(1)

        c.Update()
        data = c.GetOutputDataObject(0).GetBlock(0)
        self.assertEqual(data.GetNumberOfCells(), 75)

        rw = vtk.vtkRenderWindow()
        ren = vtk.vtkRenderer()
        rw.AddRenderer(ren)
        mapper = vtk.vtkDataSetMapper()
        mapper.SetInputData(data)
        actor = vtk.vtkActor()
        actor.SetMapper(mapper)
        ren.AddActor(actor)
        ac = ren.GetActiveCamera()
        ac.SetPosition(-7.9, 9.7, 14.6)
        ac.SetFocalPoint(3.5, 3.5, 0.5)
        ac.SetViewUp(0.08, 0.93, -0.34)
        rw.Render()
        ren.ResetCameraClippingRange()

        rtTester = vtk.vtkTesting()
        for arg in sys.argv[1:]:
            rtTester.AddArgument(arg)
        rtTester.AddArgument("-V")
        rtTester.AddArgument("tableBasedClip.png")
        rtTester.SetRenderWindow(rw)
        rw.Render()
        rtResult = rtTester.RegressionTest(10)



if __name__ == "__main__":
    Testing.main([(TestClip, 'test')])
