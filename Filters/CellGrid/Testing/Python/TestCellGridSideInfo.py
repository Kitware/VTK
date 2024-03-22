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

if __name__ == "__main__":
    Testing.main([(TestCellGridSideInfo, 'test')])
