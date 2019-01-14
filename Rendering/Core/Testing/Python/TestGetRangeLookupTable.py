#!/usr/bin/env python

import vtk
from vtk.test import Testing

class TestGetRangeLookupTable(Testing.vtkTest):
    ###
    # GetRange test
    ###
    def testGetRangeNoArg(self):
        cmap = vtk.vtkLookupTable()

        cmap.SetRange(0.0, 1.0)
        crange = cmap.GetRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 1.0)

    ###
    # GetHueRange test
    ###
    def testGetHueRangeNoArg(self):
        cmap = vtk.vtkLookupTable()

        cmap.SetHueRange(0.0, 1.0)
        crange = cmap.GetHueRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 1.0)

    ###
    # GetSaturationRange test
    ###
    def testGetSaturationRangeNoArg(self):
        cmap = vtk.vtkLookupTable()

        cmap.SetSaturationRange(0.0, 1.0)
        crange = cmap.GetSaturationRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 1.0)

    ###
    # GetAlphaRange test
    ###
    def testGetAlphaRangeNoArg(self):
        cmap = vtk.vtkLookupTable()

        cmap.SetAlphaRange(0.0, 1.0)
        crange = cmap.GetAlphaRange()
        self.assertEqual(len(crange), 2)
        self.assertEqual(crange[0], 0.0)
        self.assertEqual(crange[1], 1.0)

if __name__ == "__main__":
    Testing.main([(TestGetRangeLookupTable, 'test')])
