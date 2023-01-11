#!/usr/bin/env python

from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.test import Testing

class TestGetRangeLookupTable(Testing.vtkTest):
    ###
    # GetRange test
    ###
    def testGetRangeNoArg(self):
        cmap = vtkLookupTable()

        cmap.SetRange(0.0, 1.0)
        crange = cmap.GetRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 1.0)

    ###
    # GetHueRange test
    ###
    def testGetHueRangeNoArg(self):
        cmap = vtkLookupTable()

        cmap.SetHueRange(0.0, 1.0)
        crange = cmap.GetHueRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 1.0)

    ###
    # GetSaturationRange test
    ###
    def testGetSaturationRangeNoArg(self):
        cmap = vtkLookupTable()

        cmap.SetSaturationRange(0.0, 1.0)
        crange = cmap.GetSaturationRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 1.0)

    ###
    # GetAlphaRange test
    ###
    def testGetAlphaRangeNoArg(self):
        cmap = vtkLookupTable()

        cmap.SetAlphaRange(0.0, 1.0)
        crange = cmap.GetAlphaRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 1.0)

if __name__ == "__main__":
    Testing.main([(TestGetRangeLookupTable, 'test')])
