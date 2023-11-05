"""Test char support in VTK-Python

In C and C++, a 'char' is the smallest unit of a string.
In Python, the closest equivalent is a string of length 1.
Python has two string-like types: 'str' and 'bytes', with
the 'str' type as the most commonly used.

A VTK method that returns a 'char' will return a 'str' in Python.
A VTK parameter that takes a 'char' will require 'str' or 'bytes'
with these restrictions:
- The length must be one (or zero, for null char)
- The value must fit in 8 bits
"""

import sys
from vtkmodules.vtkCommonCore import vtkCharArray
from vtkmodules.test import Testing

class TestChar(Testing.vtkTest):
    def testUnicode(self):
        """Pass a unicode string and get it back.
        """
        a = vtkCharArray()
        a.InsertNextValue('\0')
        a.InsertNextValue('%')
        a.InsertNextValue('\u00b5') # MICRON
        a.InsertNextValue('\u00d7') # MULTIPLICATION SIGN
        c = a.GetValue(0)
        self.assertEqual(c, '\0')
        c = a.GetValue(1)
        self.assertEqual(c, '%')
        c = a.GetValue(2)
        self.assertEqual(c, '\u00b5')
        c = a.GetValue(3)
        self.assertEqual(c, '\u00d7')

    def testBytes(self):
        """Pass a bytes object, get back a unicode object
        """
        a = vtkCharArray()
        a.InsertNextValue(b'\0')
        a.InsertNextValue(b'%')
        a.InsertNextValue(b'\xb5') # MICRON
        a.InsertNextValue(b'\xd7') # MULTIPLICATION SIGN
        c = a.GetValue(0)
        self.assertEqual(c, '\0')
        c = a.GetValue(1)
        self.assertEqual(c, '%')
        c = a.GetValue(2)
        self.assertEqual(c, '\u00b5')
        c = a.GetValue(3)
        self.assertEqual(c, '\u00d7')

if __name__ == "__main__":
    Testing.main([(TestChar, 'test')])
