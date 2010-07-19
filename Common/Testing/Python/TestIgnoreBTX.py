"""Test VTK_IGNORE_BTX setting to ensure that it is ON
"""

import sys
import exceptions
import vtk
from vtk.test import Testing

class TestIgnoreBTX(Testing.vtkTest):
    def testIgnoreBTX(self):
        """Try to call a method that is BTX'd, to ensure VTK_IGNORE_BTX=ON
        """

        stringArray = vtk.vtkStringArray()
        information = vtk.vtkInformation()
        stringArray.CopyInformation(information, 0)

if __name__ == "__main__":
    Testing.main([(TestIgnoreBTX, 'test')])
