#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import vtk
import vtk.test.Testing
import math

from vtkmodules.vtkCommonCore import vtkDoubleArray, vtkIntArray, vtkUnsignedCharArray, vtkBitArray
from vtkmodules.vtkCommonDataModel import vtkHyperTreeGrid, vtkHyperTreeGridNonOrientedGeometryCursor, vtkHyperTreeGridNonOrientedCursor
from vtkmodules.vtkFiltersHyperTree import vtkHyperTreeGridGeometry

# -----------------------------------------------------------------------------
# Create Simple HTG
# -----------------------------------------------------------------------------

class TestHyperTreeGridMask(vtk.test.Testing.vtkTest):
    def testHypterTreeGridMask(self):
        "Test a simple hyper tree grid with masking"

        htg = vtkHyperTreeGrid()

        scalarArray = vtkUnsignedCharArray()
        scalarArray.SetName('scalar')
        scalarArray.SetNumberOfValues(0)

        mask = vtkBitArray()
        mask.SetName('mask')
        mask.SetNumberOfValues(22)
        mask.FillComponent(0, 0)
        htg.SetMaterialMask(mask)

        htg.GetPointData().SetScalars(scalarArray)

        htg.Initialize()
        htg.SetGridSize([3, 2, 1])
        htg.SetDimension(2)
        htg.SetBranchFactor(2)
        # specify normal index to ignore
        htg.SetOrientation(2)
        # htg.SetTransposedRootIndexing(False)

        # rectilinear grid coordinates
        xValues = vtkDoubleArray()
        xValues.SetNumberOfValues(4)
        xValues.SetValue(0, -1)
        xValues.SetValue(1, 0)
        xValues.SetValue(2, 1)
        xValues.SetValue(3, 2)
        htg.SetXCoordinates(xValues);

        yValues = vtkDoubleArray()
        yValues.SetNumberOfValues(3)
        yValues.SetValue(0, -1)
        yValues.SetValue(1, 0)
        yValues.SetValue(2, 1)
        htg.SetYCoordinates(yValues);

        zValues = vtkDoubleArray()
        zValues.SetNumberOfValues(1)
        zValues.SetValue(0, 0)
        htg.SetZCoordinates(zValues);

        # Let's split the various trees
        cursor = vtkHyperTreeGridNonOrientedCursor()
        offsetIndex = 0

        # ROOT CELL 0
        htg.InitializeNonOrientedCursor(cursor, 0, True)
        cursor.SetGlobalIndexStart(offsetIndex)

        # coarse value
        idx = cursor.GetGlobalNodeIndex()

        # GLOBAL...
        # cursor.SetGlobalIndexFromLocal(idx)

        scalarArray.InsertTuple1(idx, 0)

        # ROOT CELL 0/[0-3]
        cursor.SubdivideLeaf()
        cursor.ToChild(0)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 24)
        cursor.ToParent()

        cursor.ToChild(1)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 27)
        cursor.ToParent()

        cursor.ToChild(2)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 18)
        cursor.ToParent()

        cursor.ToChild(3)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 21)
        cursor.ToParent()

        offsetIndex += cursor.GetTree().GetNumberOfVertices()

        # ROOT CELL 1
        htg.InitializeNonOrientedCursor(cursor, 1, True)
        cursor.SetGlobalIndexStart(offsetIndex)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 15)

        # Mask
        mask.SetValue(idx, 1)

        offsetIndex += cursor.GetTree().GetNumberOfVertices()

        # ROOT CELL 2
        htg.InitializeNonOrientedCursor(cursor, 2, True)
        cursor.SetGlobalIndexStart(offsetIndex)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 2)

        # Mask
        mask.SetValue(idx, 1)

        # ROOT CELL 2/[0-3]
        cursor.SubdivideLeaf()
        cursor.ToChild(0)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 36)
        cursor.ToParent()

        cursor.ToChild(1)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 39)
        cursor.ToParent()

        cursor.ToChild(2)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 30)

        # Mask
        mask.SetValue(idx, 1)

        cursor.ToParent()

        cursor.ToChild(3)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 33)
        cursor.ToParent()

        offsetIndex += cursor.GetTree().GetNumberOfVertices()

        # ROOT CELL 3
        htg.InitializeNonOrientedCursor(cursor, 3, True)
        cursor.SetGlobalIndexStart(offsetIndex)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 0)

        offsetIndex += cursor.GetTree().GetNumberOfVertices()

        # ROOT CELL 4
        htg.InitializeNonOrientedCursor(cursor, 4, True)
        cursor.SetGlobalIndexStart(offsetIndex)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 4)

        # ROOT CELL 4/[0-3]
        cursor.SubdivideLeaf()
        cursor.ToChild(0)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 9)
        cursor.ToParent()

        cursor.ToChild(1)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 12)
        cursor.ToParent()

        cursor.ToChild(2)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 3)
        cursor.ToParent()

        cursor.ToChild(3)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 51)
        cursor.ToParent()

        # ROOT CELL 4/3/[0-3]
        cursor.ToChild(3)
        cursor.SubdivideLeaf()

        cursor.ToChild(0)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 45)
        cursor.ToParent()

        cursor.ToChild(1)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 51)
        cursor.ToParent()

        cursor.ToChild(2)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 6)
        # Mask
        mask.SetValue(idx, 1)
        cursor.ToParent()

        cursor.ToChild(3)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 45)
        cursor.ToParent()

        offsetIndex += cursor.GetTree().GetNumberOfVertices()


        # ROOT CELL 5
        htg.InitializeNonOrientedCursor(cursor, 5, True)
        cursor.SetGlobalIndexStart(offsetIndex)
        idx = cursor.GetGlobalNodeIndex()
        scalarArray.InsertTuple1(idx, 42)

        # ---------------------------------------------------------------------
        # Convert HTG to PolyData
        # ---------------------------------------------------------------------

        geometryFilter = vtkHyperTreeGridGeometry()
        geometryFilter.SetInputData(htg)

        # ---------------------------------------------------------------------
        # Rendering
        # ---------------------------------------------------------------------

        ren1 = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren1)
        iren = vtk.vtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)

        lut = vtk.vtkLookupTable()
        lut.SetHueRange(0.66, 0)
        lut.SetSaturationRange(1.0, 0.25);
        lut.SetTableRange(0, 51)
        lut.Build()

        mapper = vtk.vtkDataSetMapper()
        mapper.SetLookupTable(lut)
        mapper.SetColorModeToMapScalars()
        mapper.SetScalarModeToUseCellFieldData()
        mapper.SelectColorArray('scalar')
        mapper.SetInputConnection(geometryFilter.GetOutputPort())
        mapper.UseLookupTableScalarRangeOn()
        mapper.SetScalarRange(0, 51);

        actor = vtk.vtkActor()
        actor.SetMapper(mapper)

        ren1.AddActor(actor)
        renWin.SetSize(350,350)
        ren1.SetBackground(0.5,0.5,0.5)
        ren1.ResetCamera()
        renWin.Render()

        img_file = "TestHyperTreeGridMask.png"
        vtk.test.Testing.compareImage(renWin, vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
    vtk.test.Testing.main([(TestHyperTreeGridMask, 'test')])
