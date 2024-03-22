#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
    vtkIdList
)
from vtkmodules.vtkCommonDataModel import (
    vtkCylinder,
    vtkPlane,
    vtkRectilinearGrid,
    vtkSphere,
    vtkStructuredGrid,
    vtkUnstructuredGrid,
)
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import vtkThreshold
from vtkmodules.vtkFiltersGeometry import vtkDataSetRegionSurfaceFilter
from vtkmodules.vtkFiltersGeneral import vtkTableBasedClipDataSet
from vtkmodules.vtkFiltersParallel import vtkPPolyDataNormals
from vtkmodules.vtkIOEnSight import vtkEnSightGoldReader
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderer,
)
from vtkmodules.vtkTestingRendering import vtkTesting
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()
import sys

class TestClip(Testing.vtkTest):
    def testEmpty(self):
        ug = vtkUnstructuredGrid()
        plane = vtkPlane()
        c = vtkTableBasedClipDataSet()
        c.SetInputData(ug)
        c.SetClipFunction(plane)
        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 0)

    def testImage2DScalar(self):
        planes = ['XY', 'XZ', 'YZ']
        expectedNCells = [38, 46, 42]
        expectedNClippedCells = [104, 104, 106]
        for plane, nCells, nClippedCells in zip(planes,expectedNCells,expectedNClippedCells):
            r = vtkRTAnalyticSource()
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

            c = vtkTableBasedClipDataSet()
            c.SetInputConnection(r.GetOutputPort())
            c.SetUseValueAsOffset(0)
            c.SetValue(150)
            c.SetInsideOut(1)
            c.SetGenerateClippedOutput(1)

            c.Update()

            self.assertEqual(c.GetOutput().GetNumberOfCells(), nCells)
            self.assertEqual(c.GetClippedOutput().GetNumberOfCells(), nClippedCells)

    def testImage(self):
        r = vtkRTAnalyticSource()
        r.SetWholeExtent(-5, 5, -5, 5, -5, 5)
        r.Update()

        s = vtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = vtkTableBasedClipDataSet()
        c.SetInputConnection(r.GetOutputPort())
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

    def testRectilinear(self):
        rt = vtkRTAnalyticSource()
        rt.SetWholeExtent(-5, 5, -5, 5, -5, 5)
        rt.Update()
        i = rt.GetOutput()

        r = vtkRectilinearGrid()
        dims = i.GetDimensions()
        r.SetDimensions(dims)
        exts = i.GetExtent()
        orgs = i.GetOrigin()

        xs = vtkFloatArray()
        xs.SetNumberOfTuples(dims[0])
        for d in range(dims[0]):
            xs.SetTuple1(d, orgs[0] + exts[0] + d)
        r.SetXCoordinates(xs)

        ys = vtkFloatArray()
        ys.SetNumberOfTuples(dims[1])
        for d in range(dims[1]):
            ys.SetTuple1(d, orgs[1] + exts[2] + d)
        r.SetYCoordinates(ys)

        zs = vtkFloatArray()
        zs.SetNumberOfTuples(dims[2])
        for d in range(dims[2]):
            zs.SetTuple1(d, orgs[2] + exts[4] + d)
        r.SetZCoordinates(zs)

        s = vtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = vtkTableBasedClipDataSet()
        c.SetInputData(r)
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

    def testStructured2D(self):
        planes = ['XY', 'XZ', 'YZ']
        expectedNCells = [42, 34, 68]
        for plane, nCells in zip(planes,expectedNCells):
            rt = vtkRTAnalyticSource()
            if plane == 'XY':
                rt.SetWholeExtent(-5, 5, -5, 5, 0, 0)
            elif plane == 'XZ':
                rt.SetWholeExtent(-5, 5, 0, 0, -5, 5)
            else:
                rt.SetWholeExtent(0, 0, -5, 5, -5, 5)
            rt.Update()
            i = rt.GetOutput()

            st = vtkStructuredGrid()
            st.SetDimensions(i.GetDimensions())

            nps = i.GetNumberOfPoints()
            ps = vtkPoints()
            ps.SetNumberOfPoints(nps)
            for idx in range(nps):
                ps.SetPoint(idx, i.GetPoint(idx))

            st.SetPoints(ps)

            cyl = vtkCylinder()
            cyl.SetRadius(2)
            cyl.SetCenter(0,0,0)
            transform = vtkTransform()
            transform.RotateWXYZ(45,20,1,10)
            cyl.SetTransform(transform)

            c = vtkTableBasedClipDataSet()
            c.SetInputData(st)
            c.SetClipFunction(cyl)
            c.SetInsideOut(1)

            c.Update()

            self.assertEqual(c.GetOutput().GetNumberOfCells(), nCells)

    def testStructured(self):
        rt = vtkRTAnalyticSource()
        rt.SetWholeExtent(-5, 5, -5, 5, -5, 5)
        rt.Update()
        i = rt.GetOutput()

        st = vtkStructuredGrid()
        st.SetDimensions(i.GetDimensions())

        nps = i.GetNumberOfPoints()
        ps = vtkPoints()
        ps.SetNumberOfPoints(nps)
        for idx in range(nps):
            ps.SetPoint(idx, i.GetPoint(idx))

        st.SetPoints(ps)

        s = vtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = vtkTableBasedClipDataSet()
        c.SetInputData(st)
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

    def testUnstructured(self):
        rt = vtkRTAnalyticSource()
        rt.SetWholeExtent(-5, 5, -5, 5, -5, 5)

        t = vtkThreshold()
        t.SetInputConnection(rt.GetOutputPort())
        t.SetThresholdFunction(vtkThreshold.THRESHOLD_UPPER)
        t.SetUpperThreshold(-10.0)

        s = vtkSphere()
        s.SetRadius(2)
        s.SetCenter(0,0,0)

        c = vtkTableBasedClipDataSet()
        c.SetInputConnection(t.GetOutputPort())
        c.SetClipFunction(s)
        c.SetInsideOut(1)

        c.Update()

        self.assertEqual(c.GetOutput().GetNumberOfCells(), 64)

        eg = vtkEnSightGoldReader()
        eg.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/elements.case")
        eg.Update()

        pl = vtkPlane()
        pl.SetOrigin(3.5, 3.5, 0.5)
        pl.SetNormal(0, 0, 1)

        c.SetInputConnection(eg.GetOutputPort())
        c.SetClipFunction(pl)
        c.SetInsideOut(1)

        c.Update()
        data = c.GetOutputDataObject(0).GetBlock(0)
        self.assertEqual(data.GetNumberOfCells(), 75)

        rw = vtkRenderWindow()
        ren = vtkRenderer()
        rw.AddRenderer(ren)
        mapper = vtkDataSetMapper()
        mapper.SetInputData(data)
        actor = vtkActor()
        actor.SetMapper(mapper)
        ren.AddActor(actor)
        ac = ren.GetActiveCamera()
        ac.SetPosition(-7.9, 9.7, 14.6)
        ac.SetFocalPoint(3.5, 3.5, 0.5)
        ac.SetViewUp(0.08, 0.93, -0.34)
        rw.Render()
        ren.ResetCameraClippingRange()

        rtTester = vtkTesting()
        for arg in sys.argv[1:]:
            rtTester.AddArgument(arg)
        rtTester.AddArgument("-V")
        rtTester.AddArgument("tableBasedClip.png")
        rtTester.SetRenderWindow(rw)
        rw.Render()
        rtResult = rtTester.RegressionTest(0.05)

    def testClipOnNormal(self):
        eg = vtkEnSightGoldReader()
        eg.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/elements.case")
        eg.Update()

        rs = vtkDataSetRegionSurfaceFilter()
        rs.SetInputConnection(eg.GetOutputPort())
        rs.Update()

        n = vtkPPolyDataNormals()
        n.SetInputConnection(rs.GetOutputPort())
        n.Update()

        pl = vtkPlane()
        pl.SetOrigin(3.5, 3.5, 0.5)
        pl.SetNormal(0, 0, 1)

        c = vtkTableBasedClipDataSet()
        c.SetInputConnection(n.GetOutputPort())
        c.SetClipFunction(pl)
        c.SetInsideOut(1)

        c.Update()
        data = c.GetOutputDataObject(0).GetBlock(0)

        normals_points = data.GetPointData().GetNormals()

        idList = vtkIdList()
        # get the point id from the cell, because the point order is not deterministic
        data.GetCellPoints(9, idList)
        [x, y, z] = normals_points.GetTuple3(idList.GetId(0))
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 1)

        # get the point id from the cell, because the point order is not deterministic
        data.GetCellPoints(61, idList)
        [x, y, z] = normals_points.GetTuple3(idList.GetId(0))
        self.assertEqual(x, 0.8944272994995117)
        self.assertEqual(y, 0.44721364974975586)
        self.assertEqual(z, 0.)

if __name__ == "__main__":
    Testing.main([(TestClip, 'test')])
