"""Test enum support in VTK-Python

Created on Nov 13, 2014 by David Gobbi
"""

import sys
import vtk
from vtk.test import Testing

class TestEnum(Testing.vtkTest):
    def testGlobalNamespaceEnum(self):
        """Check that an enum in the global namespace was wrapped.
        """
        # defined in vtkGenericEnSightReader.h
        if hasattr(vtk, 'vtkGenericEnsightReader'):
            self.assertEqual(vtk.SINGLE_PROCESS_MODE, 0)
            self.assertEqual(vtk.SPARSE_MODE, 1)
            self.assertEqual(type(vtk.SINGLE_PROCESS_MODE),
                             vtk.EnsightReaderCellIdMode)
            self.assertEqual(type(vtk.SPARSE_MODE),
                             vtk.EnsightReaderCellIdMode)

    def testClassNamespaceEnum(self):
        """Check that an enum in a class namespace was wrapped.
        """
        # defined in vtkColorSeries.h
        self.assertEqual(vtk.vtkColorSeries.SPECTRUM, 0)
        self.assertEqual(type(vtk.vtkColorSeries.SPECTRUM),
                         vtk.vtkColorSeries.ColorSchemes)
        # defined in vtkErrorCode.h
        self.assertEqual(vtk.vtkErrorCode.FirstVTKErrorCode, 20000)
        self.assertEqual(type(vtk.vtkErrorCode.FirstVTKErrorCode),
                         vtk.vtkErrorCode.ErrorIds)

    def testAnonymousEnum(self):
        """Check that anonymous enums are wrapped.
        """
        # defined in vtkAbstractArray.h
        self.assertEqual(vtk.vtkAbstractArray.AbstractArray, 0)

if __name__ == "__main__":
    Testing.main([(TestEnum, 'test')])
