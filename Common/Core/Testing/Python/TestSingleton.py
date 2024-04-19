"""Test support of VTK singleton objects

Created on Aug 30, 2017 by David Gobbi

"""

import sys
from vtkmodules.vtkCommonCore import (
    vtkObject,
    vtkOutputWindow,
)
from vtkmodules.test import Testing

class TestSingleton(Testing.vtkTest):
    def testOutputWindow(self):
        a = vtkOutputWindow()
        b = vtkOutputWindow()
        self.assertNotEqual(a, b)

        c = vtkOutputWindow.GetInstance()
        d = vtkOutputWindow.GetInstance()
        self.assertIs(c, d)

    def testObject(self):
        a = vtkObject()
        b = vtkObject()
        self.assertNotEqual(a, b)

if __name__ == "__main__":
    Testing.main([(TestSingleton, 'test')])
