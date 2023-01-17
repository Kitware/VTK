"""Test enum support in VTK-Python

Created on Nov 13, 2014 by David Gobbi
"""

import sys
from vtkmodules.vtkCommonColor import vtkColorSeries
from vtkmodules.vtkCommonCore import (
    vtkAbstractArray,
    vtkEventDataAction,
    vtkEventDataForDevice,
)
from vtkmodules.vtkCommonMisc import vtkErrorCode

try:
    import vtkmodules.vtkIOEnSight as ensight
except ImportError:
    ensight is None

from vtkmodules.test import Testing

class TestEnum(Testing.vtkTest):
    def testGlobalNamespaceEnum(self):
        """Check that an enum in the global namespace was wrapped.
        """
        # defined in vtkGenericEnSightReader.h
        if ensight is not None:
            self.assertEqual(ensight.SINGLE_PROCESS_MODE, 0)
            self.assertEqual(ensight.SPARSE_MODE, 1)
            self.assertEqual(type(ensight.SINGLE_PROCESS_MODE),
                             ensight.EnsightReaderCellIdMode)
            self.assertEqual(type(ensight.SPARSE_MODE),
                             ensight.EnsightReaderCellIdMode)

    def testClassNamespaceEnum(self):
        """Check that an enum in a class namespace was wrapped.
        """
        # defined in vtkColorSeries.h
        self.assertEqual(vtkColorSeries.SPECTRUM, 0)
        self.assertEqual(type(vtkColorSeries.SPECTRUM),
                         vtkColorSeries.ColorSchemes)
        # defined in vtkErrorCode.h
        self.assertEqual(vtkErrorCode.FirstVTKErrorCode, 20000)
        self.assertEqual(type(vtkErrorCode.FirstVTKErrorCode),
                         vtkErrorCode.ErrorIds)

    def testAnonymousEnum(self):
        """Check that anonymous enums are wrapped.
        """
        # defined in vtkAbstractArray.h
        self.assertEqual(vtkAbstractArray.AbstractArray, 0)

    def testEnumClass(self):
        """Check that "enum class" members are wrapped.
        """
        # defined in vtkEventData.h
        val = vtkEventDataAction.Unknown
        self.assertEqual(val, -1)
        obj = vtkEventDataForDevice()
        self.assertEqual(obj.GetAction(), val)
        self.assertEqual(type(obj.GetAction()), vtkEventDataAction)

if __name__ == "__main__":
    Testing.main([(TestEnum, 'test')])
