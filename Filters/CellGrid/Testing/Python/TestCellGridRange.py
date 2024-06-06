# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
from vtkmodules import vtkCommonCore as cc
from vtkmodules import vtkCommonDataModel as dm
from vtkmodules import vtkFiltersCellGrid as fc
from vtkmodules import vtkIOCellGrid as io

from vtkmodules.test import Testing
import os

class TestCellGridRange(Testing.vtkTest):

    def setUp(self):
        self.reader = io.vtkCellGridReader()
        self.reader.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgTetrahedra.dg'))
        # reader.SetFileName(os.path.join(Testing.VTK_DATA_ROOT, 'Data', 'dgMixed.dg'))
        self.reader.UpdateInformation()
        self.reader.Update()
        self.source = self.reader.GetOutputDataObject(0)
        # print(self.source)

    def assertRangesIdentical(self, actual, expected):
        # TODO: Compare floating-point values with a tolerance
        self.assertAlmostEqual(expected[0], actual[0], 5, 'Unexpected range minimum.')
        self.assertAlmostEqual(expected[1], actual[1], 5, 'Unexpected range maximum.')

    def checkRangeForFile(self, filename, expectedRanges):
        self.reader.SetFileName(filename)
        self.reader.Update()
        validated = {}
        print('\n## Testing range computation on %s.\n' % filename)
        for attribute in self.source.GetCellAttributeList():
            numComp = attribute.GetNumberOfComponents()
            attributeName = attribute.GetName().Data()
            if attributeName in expectedRanges:
                validated[attributeName] = True
                expectedComponentRanges = expectedRanges[attributeName]
            else:
                expectedComponentRanges = {}
            print('### Attribute: %s, %d component(s)' % (attributeName, numComp))
            for comp in range(numComp + 2):
                cr = [1, -1]
                self.source.GetCellAttributeRange(attribute, comp - 2, cr, True)
                compName = 'L₂ norm' if comp == 0 else 'L₁ norm' if comp == 1 else '%d' % (comp - 2)
                print('  %s: [ %g, %g ]' % (compName, cr[0], cr[1]))
                if comp - 2 in expectedComponentRanges:
                    expectedRange = expectedComponentRanges[comp - 2]
                    try:
                        self.assertRangesIdentical(cr, expectedRange)
                    except AssertionError:
                        print('    ERROR: expected range [ %g, %g ]' % (expectedRange[0], expectedRange[1]))
                        validated[attributeName] = False
        # Now check that all the expected attributes were present.
        for name, compRanges in expectedRanges.items():
            if name not in validated:
                print('ERROR: Unvalidated attribute %s' % name)
        # Return success only if all the expected ranges matched:
        return all(validated.values())

    def testRange(self):
        tests = {
            # dgHexahedra.dg tests the range-responder for HCurl fields with the curl1 attribute:
            'dgHexahedra.dg': {
                'curl1':     { 0: [0, 1.39024], 1: [0, 1], 2: [-0.0487805, 0.0701754], -1: [-0.0487805, 1.39024], -2: [0, 1.4448] },
                'quadratic': { 0: [0, 6], -1: [0, 6], -2: [0, 6] },
                'scalar0':   { 0: [0, 1] },
                'scalar1':   { 0: [0, 3] },
                'scalar2':   { 0: [0, 3] },
                'scalar3':   { 0: [0, 2] },
                'shape':     { -2: [0.353553, 2.56174], -1: [0, 2], 0: [0, 2], 1: [0, 1.25], 2: [0, 1] }
                },
            'dgTetrahedra.dg': {
                'scalar0': { -2: [0, 1], -1: [0, 1], 0: [0, 1] },
                'scalar1': { -2: [0, 3], -1: [0, 3], 0: [0, 3] },
                'scalar2': { -2: [0, 2], -1: [0, 2], 0: [0, 2] },
                'scalar3': { -2: [0, 2], -1: [0, 2], 0: [0, 2] },
                'shape':   { -2: [0, 1.41421], -1: [0, 1], 0: [0, 1], 1: [0, 1], 2: [0, 0.5] }
                },
            'dgPyramids.dg': {
                'scalar0': { -2: [0, 1], -1: [0, 1], 0: [0, 1] },
                'scalar1': { -2: [0, 3], -1: [0, 3], 0: [0, 3] },
                'scalar2': { -2: [0, 2], -1: [0, 2], 0: [0, 2] },
                'scalar3': { -2: [0, 2], -1: [0, 2], 0: [0, 2] },
                'shape':   { -2: [0, 1.73205], -1: [0, 1], 0: [0, 1], 1: [0, 1], 2: [0, 1] }
                },
            # dgQuadrilateral.dg tests that the L₂ norm of the shape attribute includes
            # the origin even though no point lies at the origin:
            'dgQuadrilateral.dg': {
                'scalar0': { -2: [0, 1], -1: [0, 1], 0: [0, 1] },
                'scalar1': { -2: [0, 3], -1: [0, 3], 0: [0, 3] },
                'scalar2': { -2: [0, 2], -1: [0, 2], 0: [0, 2] },
                'scalar3': { -2: [0, 2], -1: [0, 2], 0: [0, 2] },
                'shape':   { -2: [0, 1.73205], -1: [0, 1], 0: [0, 1], 1: [0, 1], 2: [0, 1] }
                },
            'dgTriangle.dg': {
                'scalar0': { -2: [0, 1], -1: [0, 1], 0: [0, 1] },
                'scalar1': { -2: [0, 3], -1: [0, 3], 0: [0, 3] },
                'scalar2': { -2: [0, 2], -1: [0, 2], 0: [0, 2] },
                'scalar3': { -2: [0, 2], -1: [0, 2], 0: [0, 2] },
                'shape':   { -2: [0, 1], -1: [0, 1], 0: [0, 1], 1: [0, 1], 2: [0, 0.5] }
                }
            }
        ok = True
        for filename, expected in tests.items():
            if not self.checkRangeForFile(os.path.join(Testing.VTK_DATA_ROOT, 'Data', filename), expected):
                ok = False
        self.assertTrue(ok, 'Some range tests failed.')
        return ok

if __name__ == '__main__':
    Testing.main([(TestCellGridRange, 'test')])
