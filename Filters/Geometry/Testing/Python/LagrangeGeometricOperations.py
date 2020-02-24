
import os
import os.path
from vtk import *
from vtk.test import Testing
import unittest

try:
    import numpy as np
    numpyMissing = False
except:
    numpyMissing = True

renderWindowSizeMatchedRequest = None

def renderWindowTooSmall():
    global renderWindowSizeMatchedRequest
    if renderWindowSizeMatchedRequest == None:
        # Make sure we can render at the desired size:
        trw = vtkRenderWindow()
        trr = vtkRenderer()
        trr.SetBackground(1,1,1)
        trw.SetSize(300,300)
        trw.AddRenderer(trr)
        twi = vtkWindowToImageFilter()
        twi.SetInput(trw)
        twi.Update()
        tim = twi.GetOutput()
        renderWindowSizeMatchedRequest = (trw.GetSize() == tim.GetDimensions()[0:2])
        if not renderWindowSizeMatchedRequest:
            print('Skipping because RW %s != WI %s' % (trw.GetSize(), tim.GetDimensions()[0:2]))
    return not renderWindowSizeMatchedRequest

class LagrangeGeometricOperations(Testing.vtkTest):
    def setUp(self):
        self.rw = vtkRenderWindow()
        self.rr = vtkRenderer()
        self.ri = vtkRenderWindowInteractor()
        self.rw.AddRenderer(self.rr)
        self.rw.SetInteractor(self.ri)
        self.ri.Initialize()
        self.rs = self.ri.GetInteractorStyle()
        self.rs.SetCurrentStyleToTrackballCamera()
        self.rs.SetCurrentStyleToMultiTouchCamera()
        self.rr.SetBackground(1,1,1)
        self.rw.SetSize(300, 300)

        self.rdr = vtkXMLUnstructuredGridReader()
        self.rdr.SetFileName(self.pathToData('Elements.vtu'))
        print('Reading {:1}'.format(self.rdr.GetFileName()))
        self.rdr.Update()

    def addToScene(self, filt):
        ac = vtkActor()
        mp = vtkDataSetMapper()
        mp.SetInputConnection(filt.GetOutputPort())
        ac.SetMapper(mp)
        self.rr.AddActor(ac)

        return (ac, mp)

    def addSurfaceToScene(self, subdivisions=3):
        ## Surface actor
        dss = vtkDataSetSurfaceFilter()
        dss.SetInputConnection(self.rdr.GetOutputPort())
        dss.SetNonlinearSubdivisionLevel(subdivisions)

        nrm = vtkPolyDataNormals()
        nrm.SetInputConnection(dss.GetOutputPort())

        a2, m2 = self.addToScene(nrm)
        a2.GetProperty().SetOpacity(0.2)

        # wri = vtkXMLPolyDataWriter()
        # wri.SetFileName('/tmp/s2.vtp')
        # wri.SetInputConnection(nrm.GetOutputPort())
        # wri.SetDataModeToAscii()
        # wri.Write()
        return (a2, m2)

    @unittest.skipIf(renderWindowTooSmall(), 'Cannot render at requested size')
    def testContour(self):
        ## Contour actor
        con = vtkContourFilter()
        con.SetInputConnection(self.rdr.GetOutputPort())
        con.SetInputArrayToProcess(0,0,0, vtkDataSet.FIELD_ASSOCIATION_POINTS_THEN_CELLS, 'Ellipsoid')
        con.SetComputeNormals(1)
        con.SetComputeScalars(1)
        con.SetComputeGradients(1)
        con.SetNumberOfContours(4)
        con.SetValue(0, 2.5)
        con.SetValue(1, 1.5)
        con.SetValue(2, 0.5)
        con.SetValue(3, 1.05)
        con.Update()

        # Add the contour to the scene:
        a1, m1 = self.addToScene(con)
        clr = vtkColorSeries()
        lkup = vtkLookupTable()
        # Color the contours with a qualitative color scheme:
        clr.SetColorScheme(vtkColorSeries.BREWER_QUALITATIVE_DARK2)
        clr.BuildLookupTable(lkup, vtkColorSeries.CATEGORICAL);
        lkup.SetAnnotation(vtkVariant(0.5), 'Really Low')
        lkup.SetAnnotation(vtkVariant(1.05), 'Somewhat Low')
        lkup.SetAnnotation(vtkVariant(1.5), 'Medium')
        lkup.SetAnnotation(vtkVariant(2.5), 'High')
        m1.SelectColorArray('Ellipsoid')
        m1.SetLookupTable(lkup)

        a2, m2 = self.addSurfaceToScene()

        self.ri.Initialize()
        cam = self.rr.GetActiveCamera()
        cam.SetPosition(12.9377265875, 6.5914481094, 7.54647854482)
        cam.SetFocalPoint(4.38052401617, 0.925973308028, 1.91021697659)
        cam.SetViewUp(-0.491867406412, -0.115590747077, 0.862963054655)

        ## Other nice viewpoints:
        # cam.SetPosition(-1.53194314907, -6.07277748432, 19.283152654)
        # cam.SetFocalPoint(4.0, 2.25, 2.25)
        # cam.SetViewUp(0.605781341771, 0.619386648223, 0.499388772365)
        #
        # cam.SetPosition(10.5925480421, -3.08988382244, 9.2072891403)
        # cam.SetFocalPoint(4.0, 2.25, 2.25)
        # cam.SetViewUp(-0.384040517561, 0.519961374525, 0.762989547683)

        self.rw.Render()
        image = 'LagrangeGeometricOperations-Contour.png'
        # events = self.prepareTestImage(self.ri, filename=os.path.join('/tmp', image))
        Testing.compareImage(self.rw, self.pathToValidatedOutput(image))
        # Testing.interact()

        ## Write the contours out
        # wri = vtkXMLPolyDataWriter()
        # wri.SetInputConnection(con.GetOutputPort())
        # wri.SetFileName('/tmp/contours.vtp')
        # wri.Write()

    @unittest.skipIf(renderWindowTooSmall(), 'Cannot render at requested size')
    def testBoundaryExtraction(self):
        ugg = vtkUnstructuredGridGeometryFilter()
        ugg.SetInputConnection(self.rdr.GetOutputPort())
        ugg.Update()

        a1, m1 = self.addToScene(ugg)
        clr = vtkColorSeries()
        lkup = vtkLookupTable()
        # Color the contours with a qualitative color scheme:
        clr.SetColorScheme(vtkColorSeries.BREWER_QUALITATIVE_DARK2)
        clr.BuildLookupTable(lkup, vtkColorSeries.CATEGORICAL);
        lkup.SetAnnotation(vtkVariant(0), 'Cell Low')
        lkup.SetAnnotation(vtkVariant(1), 'Somewhat Low')
        lkup.SetAnnotation(vtkVariant(2), 'Medium')
        lkup.SetAnnotation(vtkVariant(3), 'High')
        m1.SetScalarModeToUseCellFieldData()
        m1.SelectColorArray('SrcCellNum')
        m1.SetLookupTable(lkup)

        self.ri.Initialize()
        cam = self.rr.GetActiveCamera()
        cam.SetPosition(16.429826228, -5.64575247779, 12.7186363446)
        cam.SetFocalPoint(4.12105459591, 1.95201869763, 1.69574200166)
        cam.SetViewUp(-0.503606926552, 0.337767269532, 0.795168746344)

        # wri = vtkXMLUnstructuredGridWriter()
        # wri.SetInputConnection(ugg.GetOutputPort())
        # wri.SetDataModeToAscii()
        # wri.SetFileName('/tmp/surface.vtu')
        # wri.Write()

        self.rw.Render()
        image = 'LagrangeGeometricOperations-Boundary.png'
        #events = self.prepareTestImage(self.ri, filename=os.path.join('/tmp', image))
        Testing.compareImage(self.rw, self.pathToValidatedOutput(image))
        # Testing.interact()

    @unittest.skipIf(renderWindowTooSmall(), 'Cannot render at requested size')
    def testClip(self):

        # Color the cells with a qualitative color scheme:
        clr = vtkColorSeries()
        lkup = vtkLookupTable()
        clr.SetColorScheme(vtkColorSeries.BREWER_QUALITATIVE_DARK2)
        clr.BuildLookupTable(lkup, vtkColorSeries.CATEGORICAL);
        lkup.SetAnnotation(vtkVariant(0), 'First cell')
        lkup.SetAnnotation(vtkVariant(1), 'Second cell')

        ## Clip
        pln = vtkPlane()
        pln.SetOrigin(4, 2, 2)
        pln.SetNormal(-0.28735, -0.67728, 0.67728)
        clp = vtkClipDataSet()
        clp.SetInputConnection(self.rdr.GetOutputPort())
        clp.SetClipFunction(pln)
        # clp.InsideOutOn()
        # clp.GenerateClipScalarsOn()
        clp.Update()

        # wri = vtkXMLUnstructuredGridWriter()
        # wri.SetFileName('/tmp/clip.vtu')
        # wri.SetInputDataObject(0, clp.GetOutputDataObject(0))
        # wri.SetDataModeToAscii()
        # wri.Write()

        # Add the clipped data to the scene:
        a1, m1 = self.addToScene(clp)
        m1.SetScalarModeToUseCellFieldData()
        m1.SelectColorArray('SrcCellNum')
        m1.SetLookupTable(lkup)

        ## Surface actor
        a2, m2 = self.addSurfaceToScene()
        m2.SetScalarModeToUseCellFieldData()
        m2.SelectColorArray('SrcCellNum')
        m2.SetLookupTable(lkup)

        self.ri.Initialize()
        cam = self.rr.GetActiveCamera()
        cam.SetPosition(16.0784261776, 11.8079343039, -6.69074553411)
        cam.SetFocalPoint(4.54685488135, 1.74152986486, 2.38091647662)
        cam.SetViewUp(-0.523934540522, 0.81705750638, 0.240644194852)
        self.rw.Render()
        image = 'LagrangeGeometricOperations-Clip.png'
        # events = self.prepareTestImage(self.ri, filename=os.path.join('/tmp', image))
        Testing.compareImage(self.rw, self.pathToValidatedOutput(image))
        # Testing.interact()

        # ri.Start()

    @unittest.skipIf(renderWindowTooSmall(), 'Cannot render at requested size')
    def testCut(self):

        # Color the cells with a qualitative color scheme:
        clr = vtkColorSeries()
        lkup = vtkLookupTable()
        clr.SetColorScheme(vtkColorSeries.BREWER_QUALITATIVE_DARK2)
        clr.BuildLookupTable(lkup, vtkColorSeries.CATEGORICAL);
        lkup.SetAnnotation(vtkVariant(0), 'First cell')
        lkup.SetAnnotation(vtkVariant(1), 'Second cell')

        ## Cuts
        pln = vtkPlane()
        pln.SetOrigin(4, 2, 2)
        pln.SetNormal(-0.28735, -0.67728, 0.67728)
        cut = vtkCutter()
        cut.SetInputConnection(self.rdr.GetOutputPort())
        cut.SetCutFunction(pln)
        cut.Update()

        #wri = vtkXMLPolyDataWriter()
        #wri.SetFileName('/tmp/cut.vtp')
        #wri.SetInputDataObject(0, cut.GetOutputDataObject(0))
        #wri.SetDataModeToAscii()
        #wri.Write()

        # Add the cut to the scene:
        a1, m1 = self.addToScene(cut)
        m1.SetScalarModeToUseCellFieldData()
        m1.SelectColorArray('SrcCellNum')
        m1.SetLookupTable(lkup)

        ## Surface actor
        a2, m2 = self.addSurfaceToScene()
        # m2.SetScalarModeToUseCellFieldData()
        # m2.SelectColorArray('SrcCellNum')
        # m2.SetLookupTable(lkup)

        self.ri.Initialize()
        cam = self.rr.GetActiveCamera()
        cam.SetPosition(16.0784261776, 11.8079343039, -6.69074553411)
        cam.SetFocalPoint(4.54685488135, 1.74152986486, 2.38091647662)
        cam.SetViewUp(-0.523934540522, 0.81705750638, 0.240644194852)
        self.rw.Render()
        image = 'LagrangeGeometricOperations-Cut.png'
        # events = self.prepareTestImage(self.ri, filename=os.path.join('/tmp', image))
        Testing.compareImage(self.rw, self.pathToValidatedOutput(image))
        # Testing.interact()

    @unittest.skipIf(numpyMissing, 'Numpy unavailable')
    @unittest.skipIf(renderWindowTooSmall(), 'Cannot render at requested size')
    def testIntersectWithLine(self):
        import numpy as np
        rn = vtkMinimalStandardRandomSequence()
        ## Choose some random lines and intersect them with our cells
        def rnums(N, vmin, vmax):
            result = []
            delta = vmax - vmin
            for i in range(N):
                result.append(rn.GetValue() * delta + vmin)
                rn.Next()
            return result
        # p1 = list(zip(rnums(10, -4,  8), rnums(10, -4, 4), rnums(10, -4, 4)))
        # p2 = list(zip(rnums(10,  0, 12), rnums(10,  0, 8), rnums(10,  0, 8)))
        p1 = [ \
            (-4, 2, 2), (2, -4, 2), (2, 2, -4), (0.125, 0.125, 4.125), (8.125, 0.125, 4.125), \
            (0.125, 0.125, 0.125), (7.875, 3.875, 3.875), \
            ] + list(zip(rnums(1000, -4,  8), rnums(1000, -4, 4), rnums(1000, -4, 4)))
        p2 = [ \
            (12, 2, 2), (2,  8, 2), (2, 2,  8), (3.45,  0.125, 4.125), (3.65,  0.125, 4.125), \
            (4.8, 4.3, 4.3), (3.3, -0.5, -0.5),
            ] + list(zip(rnums(1000,  0, 12), rnums(1000,  0, 8), rnums(1000,  0, 8)))
        lca = vtkCellArray()
        lpt = vtkPoints()
        nli = len(p1)
        [lpt.InsertNextPoint(x) for x in p1]
        [lpt.InsertNextPoint(x) for x in p2]
        [lca.InsertNextCell(2, [i, nli + i]) for i in range(nli)]
        lpd = vtkPolyData()
        lpd.SetPoints(lpt)
        lpd.SetLines(lca)
        # tub = vtkTubeFilter()
        # tub.SetInputDataObject(0, lpd)
        # tub.SetVaryRadiusToVaryRadiusOff()
        # tub.SetRadius(0.025)
        # tub.SetCapping(1)
        # tub.SetNumberOfSides(16)
        # al, ml = self.addToScene(tub)
        # al.GetProperty().SetColor(0.5, 0.5, 0.5)

        ug = self.rdr.GetOutputDataObject(0)
        from vtkmodules.vtkCommonCore import mutable
        tt = mutable(0)
        subId = mutable(-1)
        xx = [0,0,0]
        rr = [0,0,0]
        ipt = vtkPoints()
        ica = vtkCellArray()
        rst = [vtkDoubleArray(), vtkDoubleArray(), vtkDoubleArray()]
        pname = ['R', 'S', 'T']
        [rst[i].SetName(pname[i]) for i in range(3)]
        for cidx in range(ug.GetNumberOfCells()):
            cell = ug.GetCell(cidx)
            order = [cell.GetOrder(i) for i in range(cell.GetCellDimension())]
            npts = 1
            for o in order:
                npts = npts * (o + 1)
            weights = np.zeros((npts,1));
            # print('Cell {:1}'.format(cidx))
            for pp in range(len(p1)):
                # print('  Line {p1} -- {p2}'.format(p1=p1[pp], p2=p2[pp]))
                done = False
                xp1 = np.array(p1[pp], dtype=np.float64)
                xp2 = np.array(p2[pp], dtype=np.float64)
                while not done:
                    done = not cell.IntersectWithLine(xp1, xp2, 1e-8, tt, xx, rr, subId)
                    # print('  Hit: {hit}  @{t} posn {posn} param {rr} subId {subId}'.format( \
                    #    hit=not done, t=tt.get(), posn=xx, rr=rr, subId=subId.get()))
                    if not done:
                        pid = [ipt.InsertNextPoint(xx)]
                        ica.InsertNextCell(1, pid)
                        [rst[i].InsertNextTuple([rr[i],]) for i in range(3)]
                        delta = xp2 - xp1
                        mag = np.sqrt(np.sum(delta * delta))
                        # Note: we use the single-precision epsilon below because
                        #       some arithmetic in IntersectWithLine appears to be
                        #       done at low precision or with a large tolerance.
                        xp1 = np.array(xx) + (delta / mag) * np.finfo(np.float32).eps

        ipd = vtkPolyData()
        ipd.SetPoints(ipt)
        ipd.SetVerts(ica)
        [ipd.GetPointData().AddArray(rst[i]) for i in range(3)]
        print('{N} vertices to glyph'.format(N=ipd.GetNumberOfCells()))
        gly = vtkGlyph3D()
        ssc = vtkSphereSource()
        gly.SetSourceConnection(ssc.GetOutputPort())
        gly.SetInputDataObject(0, ipd)
        gly.SetScaleFactor(0.15)
        gly.FillCellDataOn()
        ai, mi = self.addToScene(gly)
        # ai.GetProperty().SetColor(0.8, 0.3, 0.3)
        clr = vtkColorSeries()
        lkup = vtkLookupTable()
        clr.SetColorScheme(vtkColorSeries.BREWER_SEQUENTIAL_BLUE_PURPLE_9)
        clr.BuildLookupTable(lkup, vtkColorSeries.ORDINAL);
        lkup.SetRange(0,1)
        mi.SetScalarModeToUseCellFieldData()
        mi.SelectColorArray('R')
        mi.SetLookupTable(lkup)

        #wri = vtkXMLPolyDataWriter()
        #wri.SetFileName('/tmp/s2.vtp')
        #gly.Update()
        #wri.SetInputDataObject(0, gly.GetOutputDataObject(0))
        #wri.SetDataModeToAscii()
        #wri.Write()

        ## Surface actor
        a2, m2 = self.addSurfaceToScene(1)

        a3, m3 = self.addSurfaceToScene(1)
        a3.GetProperty().SetRepresentationToWireframe()

        ## Render test scene
        self.ri.Initialize()
        cam = self.rr.GetActiveCamera()
        # cam.SetPosition(4.14824823557, -15.3201939164, 7.48529277914)
        # cam.SetFocalPoint(4.0392921746, 2.25197875899, 1.59174422348)
        # cam.SetViewUp(-0.00880943634729, 0.317921564576, 0.948076090095)
        cam.SetPosition(14.9792978813, -9.28884906174, 13.1673942646)
        cam.SetFocalPoint(3.76340069188, 2.13047224356, 1.73084897464)
        cam.SetViewUp(-0.0714929546473, 0.669898141926, 0.739002866625)
        self.rr.ResetCameraClippingRange()

        for color in ['R', 'S', 'T']:
          mi.SelectColorArray(color)
          self.rw.Render()
          image = 'LagrangeGeometricOperations-Stab{c}.png'.format(c=color)
          # events = self.prepareTestImage(self.ri, filename=os.path.join('/tmp', image))
          Testing.compareImage(self.rw, self.pathToValidatedOutput(image))

if __name__ == "__main__":
    Testing.main([(LagrangeGeometricOperations, 'test')])
