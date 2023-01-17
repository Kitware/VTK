"""Test the wrapping of parameters that are pointers to numeric types

Created on Nov 11, 2013 by David Gobbi
"""

import sys
from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkVariant,
)
from vtkmodules.test import Testing

class TestPointers(Testing.vtkTest):
    def testBoolPointer(self):
        v = vtkVariant("1")
        bp = [False]
        d = v.ToFloat(bp)
        self.assertEqual(bp, [True])
        v = vtkVariant("George")
        d = v.ToFloat(bp)
        self.assertEqual(bp, [False])

    def testDoublePointer(self):
        dp = [5.2]
        vtkMath.ClampValue(dp, (-0.5, 0.5))
        self.assertEqual(dp, [0.5])
        dp = [5.2, 1.0, -0.2, 0.3, 10.0, 6.0]
        vtkMath.ClampValues(dp, len(dp), (-0.5, 0.5), dp)
        self.assertEqual(dp, [0.5, 0.5, -0.2, 0.3, 0.5, 0.5])

if __name__ == "__main__":
    Testing.main([(TestPointers, 'test')])
