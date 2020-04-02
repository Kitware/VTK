#!/usr/bin/env python
import sys
import vtk
from vtk.test import Testing

# This script creates an unstructured grid of 4 quadratic quads
# with vtkIdTypeArray and vtkFloatArray point data and vtkIdTypeArray cell data.
#
# When vtkDataSetSurfaceFilter has sub-division level < 2 there is no
# interpolation being done so the point-data idtype array is expected to be copied

class TestDataSetSurfaceFilter(Testing.vtkTest):
    def setUp(self):
        # corner-points (9)
        c0=[[0,0,0],[1,0,0],[2,0,0]]
        c1=[[0,1,0],[1,1,0],[2,1,0]]
        c2=[[0,2,0],[1,2,0],[2,2,0]]
        # mid nodes on horizontal edges (6)
        mx0=[[.5,0,0],[1.5,0,0]]
        mx1=[[.5,1,0],[1.5,1,0]]
        mx2=[[.5,2,0],[1.5,2,0]]
        # mid nodes on vertical edges (6)
        my0=[[0,.5,0],[1,.5,0],[2,.5,0]]
        my1=[[0,1.5,0],[1,1.5,0],[2,1.5,0]]
        #
        # create points and point data
        pts=vtk.vtkPoints()
        pts.Allocate(21)
        self.point_ids=vtk.vtkIdTypeArray()
        self.point_ids.SetName('point-ids')
        self.point_values=vtk.vtkFloatArray()
        self.point_values.SetName('point-values')
        i=1
        for coords in (c0,c1,c2,mx0,mx1,mx2,my0,my1):
            for xyz in coords:
                pts.InsertNextPoint(xyz[0],xyz[1],xyz[2])
                self.point_ids.InsertNextValue(i+100)
                i+=1
                self.point_values.InsertNextValue( float(i) / 10 )
        #
        # create cells and cell data
        corners=[(0,1,4,3),
                 (1,2,5,4),
                 (3,4,7,6),
                 (4,5,8,7)]
        midnodes=[(9+0,15+1,9+2,15+0),
                  (9+1,15+2,9+3,15+1),
                  (9+2,15+4,9+4,15+3),
                  (9+3,15+5,9+5,15+4)]
        self.ug=vtk.vtkUnstructuredGrid()
        self.ug.SetPoints(pts)
        self.ug.Allocate(4)
        self.cell_ids=vtk.vtkIdTypeArray()
        self.cell_ids.SetName('cell-ids')
        i=1
        for (c,m) in zip(corners,midnodes):
            self.ug.InsertNextCell(vtk.VTK_QUADRATIC_QUAD,8,(c+m))
            self.cell_ids.InsertNextValue(i+1000)
            i+=1
        self.ug.GetPointData().AddArray(self.point_ids)
        self.ug.GetPointData().AddArray(self.point_values)
        self.ug.GetCellData().AddArray(self.cell_ids)
        #
        # create the surface-filter
        surface=vtk.vtkDataSetSurfaceFilter()
        surface.SetInputData(self.ug)
        surface.Update()

        self.output=surface.GetOutput()

    def testNumberOfPoints(self):
        self.assertEqual(self.output.GetNumberOfPoints(),
                         self.ug.GetNumberOfPoints())

    def testPointIds(self):
        out_point_ids=self.output.GetPointData().GetArray('point-ids')
        self.assertTrue(out_point_ids)
        self.assertEqual(out_point_ids.GetNumberOfTuples(),
                         self.point_ids.GetNumberOfTuples())
        self.assertEqual(out_point_ids.GetRange(),
                         self.point_ids.GetRange())

    def testPointValues(self):
        out_point_values=self.output.GetPointData().GetArray('point-values')
        self.assertTrue(out_point_values)
        self.assertEqual(out_point_values.GetNumberOfTuples(),
                         self.point_values.GetNumberOfTuples())
        self.assertEqual(out_point_values.GetRange(),
                         self.point_values.GetRange())

    def testCellIds(self):
        out_cell_ids=self.output.GetCellData().GetArray('cell-ids')
        self.assertTrue(out_cell_ids)
        self.assertGreater(out_cell_ids.GetNumberOfTuples(),
                           self.cell_ids.GetNumberOfTuples())
        self.assertEqual(out_cell_ids.GetRange(),
                         self.cell_ids.GetRange())

if __name__ == "__main__":
    Testing.main([(TestDataSetSurfaceFilter,'test')])
