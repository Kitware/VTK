"""Test string and unicode support in VTK-Python

The following string features have to be tested for string and unicode
- Pass a string arg by value
- Pass a string arg by reference
- Return a string arg by value
- Return a string arg by reference

The following features are not supported
- Pointers to strings, arrays of strings
- Passing a string arg by reference and returning a value in it

Created on May 12, 2010 by David Gobbi
"""

import sys
from vtkmodules.vtkCommonCore import (
    VTK_INT,
    VTK_STRING,
    vtkArray,
    vtkStringArray,
)
from vtkmodules.test import Testing

cedilla = 'Fran\xe7ois'
nocedilla = 'Francois'
eightbit = 'Francois'.encode('ascii')

class TestString(Testing.vtkTest):
    def testPassByValue(self):
        """Pass string by value... hard to find examples of this,
        because "const char *" methods shadow "vtkStdString" methods.
        """
        self.assertEqual('y', 'y')

    def testReturnByValue(self):
        """Return a string by value."""
        a = vtkArray.CreateArray(1, VTK_INT)
        a.Resize(1,1)
        a.SetDimensionLabel(0, 'x')
        s = a.GetDimensionLabel(0)
        self.assertEqual(s, 'x')

    def testPassByReference(self):
        """Pass a string by reference."""
        a = vtkArray.CreateArray(0, VTK_STRING)
        a.SetName("myarray")
        s = a.GetName()
        self.assertEqual(s, "myarray")

    def testReturnByReference(self):
        """Return a string by reference."""
        a = vtkStringArray()
        s = "hello"
        a.InsertNextValue(s)
        t = a.GetValue(0)
        self.assertEqual(t, s)

    def testPassAndReturnUnicodeByReference(self):
        """Pass a unicode string by const reference"""
        a = vtkStringArray()
        a.InsertNextValue(cedilla)
        u = a.GetValue(0)
        self.assertEqual(u, cedilla)

    def testPassUnicodeAsString(self):
        """Pass unicode where string is expected.  Should succeed."""
        a = vtkStringArray()
        a.InsertNextValue(nocedilla)
        s = a.GetValue(0)
        self.assertEqual(s, 'Francois')

    def testPassBytesAsString(self):
        """Pass 8-bit string where string is expected.  Should succeed."""
        a = vtkStringArray()
        a.InsertNextValue(eightbit)
        s = a.GetValue(0)
        self.assertEqual(s, 'Francois')

    def testPassEncodedString(self):
        """Pass encoded 8-bit strings."""
        a = vtkStringArray()
        # latin1 encoded string will be returned as "bytes"
        encoded = cedilla.encode('latin1')
        a.InsertNextValue(encoded)
        result = a.GetValue(0)
        self.assertEqual(type(result), bytes)
        self.assertEqual(result, encoded)
        # utf-8 encoded string will be returned as "str"
        a = vtkStringArray()
        encoded = cedilla.encode('utf-8')
        a.InsertNextValue(encoded)
        result = a.GetValue(0)
        self.assertEqual(type(result), str)
        self.assertEqual(result.encode('utf-8'), encoded)

if __name__ == "__main__":
    Testing.main([(TestString, 'test')])
