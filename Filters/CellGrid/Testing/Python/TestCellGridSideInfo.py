# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkCommonExecutionModel as em
from vtkmodules import vtkImagingCore as ic
from vtkmodules import vtkFiltersCore as ff
from vtkmodules import vtkFiltersCellGrid as fc
from vtkmodules import vtkIOCellGrid as io
from vtkmodules import vtkIOXML as ix
from vtkmodules import vtkIOIOSS as ii
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

from vtkmodules.vtkRenderingCore import *
from vtkmodules.vtkRenderingOpenGL2 import *
from vtkmodules.vtkRenderingCellGrid import *
from vtkmodules.vtkInteractionStyle import *
from vtkmodules.vtkInteractionWidgets import vtkCameraOrientationWidget

# We need vtkIOExodus as otherwise an information key will be missing, generating
# the following warning:
#   Could not locate key vtkExodusIIReader::GLOBAL_TEMPORAL_VARIABLE.
from vtkmodules import vtkIOExodus as ie

from vtkmodules.test import Testing
import os

class TestCellGridSideInfo(Testing.vtkTest):

    def findChildSidesOfSide(self, cell, side, sideConn, sideDim, sideType):
        """Given a side of a cell, return a list of sides of that side."""
        nt = cell.GetNumberOfSideTypes()
        childSides = []
        ncc = 2 if len(sideConn) > 2 else len(sideConn) - 1
        augSideConn = sideConn + sideConn
        for ii in range(len(sideConn)):
            childConnToFind = set(augSideConn[ii:ii+ncc])
            # print('Looking for side', childConnToFind)
            for childSideType in range(sideType + 1, nt):
                childSideRange = cell.GetSideRangeForSideType(childSideType)
                childSideShape = cell.GetSideShape(childSideRange[0])
                childSideDim = fc.vtkDGCell.GetShapeDimension(childSideShape)
                if childSideDim != sideDim - 1:
                    continue
                found = False
                for childSide in range(childSideRange[0], childSideRange[1]):
                    childSideConn = cell.GetSideConnectivity(childSide)
                    if set(childSideConn) == childConnToFind:
                        childSides += [childSide,]
                        found = True
                        break
                if found:
                    break
        return childSides

    def computeAllCellSides(self, cell):
        """Given an instance of some subclass of vtkDGCell, return an array
        holding the sides of each of its sides, starting with the sides of
        the cell itself.

        Note that this method can be used to compute the values for any
        cell's SidesOfSides array.
        """
        nc = cell.GetNumberOfCorners()
        cellConn = [x for x in range(nc)]
        cellDim = cell.GetDimension()
        nt = cell.GetNumberOfSideTypes()
        # Generate the immediate sides of the cell itself (not sides of the cell).
        # This is always an integer sequence from 0 to the number of sides of dimension cellDim - 1.
        immCellSides = []
        for sideType in range(nt):
            sideRange = cell.GetSideRangeForSideType(sideType)
            sideShape = cell.GetSideShape(sideRange[0])
            sideDim = fc.vtkDGCell.GetShapeDimension(sideShape)
            if sideDim == cellDim - 1:
                immCellSides += [ss for ss in range(sideRange[0], sideRange[1])]
            else:
                break
        # Now use permutations of each lower-dimensional-side's connectivity to
        # identify children of its sides.
        cellSides = [tuple(immCellSides),]
        for sideType in range(nt):
            sideRange = cell.GetSideRangeForSideType(sideType)
            sideShape = cell.GetSideShape(sideRange[0])
            sideDim = fc.vtkDGCell.GetShapeDimension(sideShape)
            for side in range(*sideRange):
                sideConn = cell.GetSideConnectivity(side)
                # print('Side ', side, 'conn', sideConn)
                cellSides += (tuple(self.findChildSidesOfSide(cell, side, sideConn, sideDim, sideType)),)
        return cellSides

    def sideOffsetCheck(self, cellType, expected):
        nst = cellType.GetNumberOfSideTypes()
        print('  Side offsets for ', cellType.GetClassName())
        actual = [cellType.GetSideRangeForSideType(x) for x in range(-2,nst)]
        print('    actual  ', actual)
        print('    expected', expected)
        self.assertEqual(actual, expected)

    def sideShapeCheck(self, cellType, expected):
        nst = cellType.GetNumberOfSideTypes()
        print('  Side shapes for ', cellType.GetClassName())
        actual = [ \
            cellType.GetShapeName( \
                cellType.GetSideShape( \
                    cellType.GetSideRangeForSideType(x)[0])).Data() for x in range(-1, nst)]
        print('    actual  ', actual)
        print('    expected', expected)
        self.assertEqual(actual, expected)
        # Regression test: ensure calling with out-of-range side ID does not crash
        badSideShape = cellType.GetSideShape(cellType.GetSideRangeForSideType(-2)[1])
        self.assertEqual(badSideShape, fc.vtkDGCell.None_)

    def classListCheck(self, module, expectedCellTypes):
        print('Ensuring all DG cells in ', module.__name__, ' are tested.')
        import inspect
        actual = set()
        for memberName, memberObj in inspect.getmembers(module):
            if inspect.isclass(memberObj):
                try:
                    obj = memberObj()
                    if obj.IsA('vtkDGCell') == 1 and obj.GetClassName() != 'vtkDGCell':
                        actual.add(memberName)
                except:
                    pass
        print('  actual  ', actual)
        print('  expected', expectedCellTypes)
        self.assertEqual(actual, expectedCellTypes)


    def testSideOffsets(self):
        """Test that offsets into lists of sides of a given type are correct.
           Test that shapes of cell-sides are correct."""
        print('Testing side type/numbering consistency.')
        self.sideOffsetCheck(fc.vtkDGEdge(), [(0, 2), (-1, 0), (0, 2)])
        self.sideOffsetCheck(fc.vtkDGHex(), [(0, 26), (-1, 0), (0, 6), (6, 18), (18, 26)])
        self.sideOffsetCheck(fc.vtkDGPyr(), [(0, 18), (-1, 0), (0, 4), (4, 5), (5, 13), (13, 18)])
        self.sideOffsetCheck(fc.vtkDGQuad(), [(0, 8), (-1, 0), (0, 4), (4, 8)])
        self.sideOffsetCheck(fc.vtkDGTet(), [(0, 14), (-1, 0), (0, 4), (4, 10), (10, 14)])
        self.sideOffsetCheck(fc.vtkDGTri(), [(0, 6), (-1, 0), (0, 3), (3, 6)])
        self.sideOffsetCheck(fc.vtkDGVert(), [(0, 0), (-1, 0)])
        self.sideOffsetCheck(fc.vtkDGWdg(), [(0, 20), (-1, 0), (0, 3), (3, 5), (5, 14), (14, 20)])

        print('Testing side shape consistency.')
        self.sideShapeCheck(fc.vtkDGEdge(), ['edge', 'vertex'])
        self.sideShapeCheck(fc.vtkDGHex(), ['hexahedron', 'quadrilateral', 'edge', 'vertex'])
        self.sideShapeCheck(fc.vtkDGPyr(), ['pyramid', 'triangle', 'quadrilateral', 'edge', 'vertex'])
        self.sideShapeCheck(fc.vtkDGQuad(), ['quadrilateral', 'edge', 'vertex'])
        self.sideShapeCheck(fc.vtkDGTet(), ['tetrahedron', 'triangle', 'edge', 'vertex'])
        self.sideShapeCheck(fc.vtkDGTri(), ['triangle', 'edge', 'vertex'])
        self.sideShapeCheck(fc.vtkDGVert(), ['vertex'])
        self.sideShapeCheck(fc.vtkDGWdg(), ['wedge', 'quadrilateral', 'triangle', 'edge', 'vertex'])

        # Check the module to ensure no new vtkDGCell subclasses are added without a test.
        cellInst = (
            fc.vtkDGEdge(), fc.vtkDGHex(), fc.vtkDGPyr(), fc.vtkDGQuad(),
            fc.vtkDGTet(), fc.vtkDGTri(), fc.vtkDGVert(), fc.vtkDGWdg()
        )
        cellTypes = set(['vtkDGEdge', 'vtkDGHex', 'vtkDGPyr', 'vtkDGQuad', 'vtkDGTet', 'vtkDGTri', 'vtkDGVert', 'vtkDGWdg'])
        self.classListCheck(fc, cellTypes)

        for cell in cellInst:
            allCellSides = self.computeAllCellSides(cell)
            print(cell.GetClassName(), '\n'.join([str(x) for x in allCellSides]))
            for parent in range(-1, len(allCellSides) - 1):
                self.assertEqual(cell.GetSidesOfSide(parent), tuple(allCellSides[parent + 1]))

    def checkSideRoundTrip(self, pipeline, rdr, cg1, cg2, filename, cellType, expected):
        """Check that the 'expected' cell-/side-sources are preserved in an I/O round trip."""
        fullFilename = os.path.join(Testing.VTK_DATA_ROOT, 'Data', filename)
        pipeline.first.SetFileName(fullFilename)
        pipeline.update()
        print('  Testing cells of type', cellType)
        rdr.Modified()
        rdr.Update()
        ihex = cg1.GetCellType(cellType)
        ohex = cg2.GetCellType(cellType)
        self.assertTrue(ihex != None, 'No input cells of type ' + cellType)
        self.assertTrue(ohex != None, 'No output cells of type ' + cellType)
        self.assertEqual(ihex.GetNumberOfCellSources() + 1, len(expected),
                         'Number of cell sources is unexpected.')
        self.assertEqual(ihex.GetNumberOfCellSources(), ohex.GetNumberOfCellSources(),
                         'Should have the same number of cell/side types.')
        for sourceIdx in range(-1, ihex.GetNumberOfCellSources()):
            print('    Source', sourceIdx)
            self.assertTrue(sourceIdx in expected, 'Unexpected source index')
            self.assertEqual(ihex.GetCellSourceConnectivity(sourceIdx).GetNumberOfTuples(),
                             expected[sourceIdx][0], 'Unexpected number of cells/sides.')
            self.assertEqual(ihex.GetCellSourceShape(sourceIdx), expected[sourceIdx][1],
                             'Unexpected side shape.')
            self.assertEqual(ihex.GetCellSourceOffset(sourceIdx), expected[sourceIdx][2],
                             'Unexpected offsets.')
            self.assertEqual(ihex.GetCellSourceIsBlanked(sourceIdx), expected[sourceIdx][3],
                             'Unexpected blanking.')
            self.assertEqual(ihex.GetCellSourceShape(sourceIdx), ohex.GetCellSourceShape(sourceIdx),
                             'Should have the same shape.')
            self.assertEqual(ihex.GetCellSourceSideType(sourceIdx), ohex.GetCellSourceSideType(sourceIdx),
                             'Should have the same side-type.')
            self.assertEqual(ihex.GetCellSourceOffset(sourceIdx), ohex.GetCellSourceOffset(sourceIdx),
                             'Should have the same offset.')
            self.assertEqual(ihex.GetCellSourceIsBlanked(sourceIdx), ohex.GetCellSourceIsBlanked(sourceIdx),
                             'Should have the same blanking.')
            self.assertEqual(
                ihex.GetCellSourceConnectivity(sourceIdx).GetNumberOfTuples(),
                ohex.GetCellSourceConnectivity(sourceIdx).GetNumberOfTuples(),
                'Should have the same number of cells.')

    def testSideRoundTrip(self):
        """Test that side-cells are properly round-tripped by the reader and writer."""
        print('Testing that sides are preserved in an I/O round trip.')
        inputGeometry = os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgHexahedra.dg')
        outputGeometry = os.path.join(Testing.VTK_TEMP_DIR, 'testAllSides.dg')
        ini = io.vtkCellGridReader(file_name=inputGeometry)
        sid = fc.vtkCellGridComputeSides(
                output_dimension_control=dm.vtkCellGridSidesQuery.AllSides,
                preserve_renderable_inputs=False,
                omit_sides_for_renderable_inputs=False)
        wri = io.vtkCellGridWriter(file_name=outputGeometry)
        sidePass = ini >> sid >> wri
        sidePass.update()
        cg1 = sid.GetOutputDataObject(0)

        rdr = io.vtkCellGridReader(file_name=outputGeometry)
        rdr.Update()
        cg2 = rdr.GetOutputDataObject(0)

        # Format for the expected value dictionary passed to checkSideRoundTrip:
        #    sideSpecIndex: (number of cells, shape enum, offset, blanking)
        self.checkSideRoundTrip(sidePass, rdr, cg1, cg2, 'dgHexahedra.dg', 'vtkDGHex', {
            -1: ( 2, 5,  0, True),
             0: (10, 3,  0, False),
             1: (20, 1, 10, False),
             2: (12, 0, 30, False)})
        self.checkSideRoundTrip(sidePass, rdr, cg1, cg2, 'dgTetrahedra.dg', 'vtkDGTet', {
            -1: (2, 4,  0, True),
             0: (6, 2,  0, False),
             1: (9, 1,  6, False),
             2: (5, 0, 15, False)})
        self.checkSideRoundTrip(sidePass, rdr, cg1, cg2, 'dgWedges.dg', 'vtkDGWdg', {
            -1: ( 2, 6,  0, True),
             0: ( 4, 3,  0, False),
             1: ( 4, 2,  4, False),
             2: (14, 1,  8, False),
             3: ( 8, 0, 22, False)})
        self.checkSideRoundTrip(sidePass, rdr, cg1, cg2, 'dgPyramids.dg', 'vtkDGPyr', {
            -1: ( 2, 7,  0, True),
             0: ( 2, 3,  0, False),
             1: ( 6, 2,  2, False),
             2: (13, 1,  8, False),
             3: ( 7, 0, 21, False)})
        self.checkSideRoundTrip(sidePass, rdr, cg1, cg2, 'dgQuadrilateral.dg', 'vtkDGQuad', {
            -1: ( 2, 3,  0, True),
             0: ( 6, 1,  0, False),
             1: ( 6, 0,  6, False)})
        self.checkSideRoundTrip(sidePass, rdr, cg1, cg2, 'dgTriangle.dg', 'vtkDGTri', {
            -1: ( 2, 2,  0, True),
             0: ( 4, 1,  0, False),
             1: ( 4, 0,  4, False)})
        self.checkSideRoundTrip(sidePass, rdr, cg1, cg2, 'dgEdges.dg', 'vtkDGEdge', {
            -1: ( 5, 1,  0, True),
             0: ( 2, 0,  0, False)})

if __name__ == "__main__":
    Testing.main([(TestCellGridSideInfo, 'test')])
