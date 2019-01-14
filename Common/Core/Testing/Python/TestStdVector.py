"""Test the wrapping of std::vector

Created on Jul 28, 2018 by David Gobbi
"""

import sys
import vtk
from vtk.test import Testing

class TestStdVector(Testing.vtkTest):
    def testVectorReturn(self):
        u = vtk.vtkUnicodeString(u'hello world')
        v = u.utf16_str()
        self.assertEqual(tuple(v), tuple([ord(c) for c in 'hello world']))

    def testVectorArg(self):
        u = vtk.vtkUnicodeString(u'hello world')
        v = []
        a = u.utf16_str(v)
        self.assertEqual(v, list([ord(c) for c in 'hello world']))
        b = bytearray()
        a = u.utf16_str(b)
        self.assertEqual(b, bytearray(b'hello world'))

if __name__ == "__main__":
    Testing.main([(TestStdVector, 'test')])
