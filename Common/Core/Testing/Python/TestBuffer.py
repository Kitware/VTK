"""Test buffer protocol for VTK arrays

Python 2.6 introduced a new buffer protocol that can expose raw
memory as a multi-dimensional array.  This is used, for example,
by numpy in order to automatically generate arrays from memory
exposed by other python extension modules.

Created on Aug 1, 2015 by David Gobbi

"""

import sys
import struct
import vtkmodules.vtkCommonCore
from vtkmodules.vtkCommonCore import (
    VTK_SIZEOF_ID_TYPE,
    VTK_SIZEOF_LONG,
    buffer_shared,
    vtkBitArray,
    vtkCharArray,
    vtkFloatArray,
)
from vtkmodules.test import Testing

# array types and the corresponding format
lsize = VTK_SIZEOF_LONG
idsize = VTK_SIZEOF_ID_TYPE
if idsize == 8:
    idchar = 'q'
else:
    idchar = 'i'

arrayType = {
 'SignedChar':('b', 1), 'UnsignedChar':('B', 1),
 'Short':('h', 2), 'UnsignedShort':('H', 2),
 'Int':('i', 4), 'UnsignedInt':('I', 4),
 'Long':('l', lsize), 'UnsignedLong':('L', lsize),
 'IdType':(idchar, idsize),
 'LongLong':('q', 8), 'UnsignedLongLong':('Q', 8)
 }

class TestBuffer(Testing.vtkTest):
    def testOneDimensionalDataArray(self):
        """Test one-dimensional data array."""
        for atype,ainfo in arrayType.items():
            aclass = getattr(vtkmodules.vtkCommonCore, 'vtk' + atype + 'Array')
            a = aclass()
            a.InsertNextValue(10)
            a.InsertNextValue(7)
            a.InsertNextValue(85)
            m = memoryview(a)
            self.assertEqual(m.format, ainfo[0])
            self.assertEqual(m.itemsize, ainfo[1])
            self.assertEqual(m.strides, (ainfo[1],))
            self.assertEqual(m.shape, (3,))
            self.assertEqual(m.ndim, 1)
            # test the contents of the memoryview
            tp = struct.unpack(3*ainfo[0], m.tobytes())
            self.assertEqual(tp, (10, 7, 85))
            # now test re-creating the array from a buffer
            b = aclass()
            b.SetVoidArray(m, 3, True)
            self.assertEqual(b.GetValue(0), 10)
            self.assertEqual(b.GetValue(1), 7)
            self.assertEqual(b.GetValue(2), 85)

    def testTwoDimensionalDataArray(self):
        """Test data array with components."""
        for atype,ainfo in arrayType.items():
            aclass = getattr(vtkmodules.vtkCommonCore, 'vtk' + atype + 'Array')
            a = aclass()
            a.SetNumberOfComponents(3)
            a.InsertNextTuple((10, 7, 4))
            a.InsertNextTuple((85, 8, 2))
            m = memoryview(a)
            self.assertEqual(m.format, ainfo[0])
            self.assertEqual(m.itemsize, ainfo[1])
            self.assertEqual(m.shape, (2, 3))
            self.assertEqual(m.strides, (ainfo[1]*3, ainfo[1]))
            self.assertEqual(m.ndim, 2)
            # test the contents of the memoryview
            tp = struct.unpack(6*ainfo[0], m.tobytes())
            self.assertEqual(tp, (10, 7, 4, 85, 8, 2))

    def testCharArray(self):
        """Test the special case of the char array."""
        # bit array is actually stored as a byte array
        a = vtkCharArray()
        a.SetNumberOfComponents(5)
        a.InsertNextTypedTuple("hello")
        a.InsertNextTypedTuple("world")
        m = memoryview(a)
        self.assertEqual(m.format, 'c')
        self.assertEqual(m.itemsize, 1)
        self.assertEqual(m.shape, (2, 5))
        self.assertEqual(m.strides, (5, 1))
        self.assertEqual(m.ndim, 2)
        # test the contents of the memoryview
        self.assertEqual(m.tobytes(), b"helloworld")

    def testBitArray(self):
        """Test the special case of the bit array."""
        # bit array is actually stored as a byte array
        a = vtkBitArray()
        a.InsertNextValue(0)
        a.InsertNextValue(1)
        a.InsertNextValue(1)
        a.InsertNextValue(0)
        a.InsertNextValue(1)
        m = memoryview(a)
        self.assertEqual(m.format, 'B')
        self.assertEqual(m.itemsize, 1)
        self.assertEqual(m.shape, (1,))
        # test the contents of the memoryview
        self.assertEqual(ord(m.tobytes()) & 0xF8, 0x68)

    def testBufferShared(self):
        """Test the special buffer_shared() check that VTK provides."""
        a = bytearray(b'hello')
        self.assertEqual(buffer_shared(a, a), True)
        b = bytearray(b'hello')
        self.assertEqual(buffer_shared(a, b), False)

        a = vtkFloatArray()
        a.SetNumberOfComponents(3)
        a.InsertNextTuple((10, 7, 4))
        a.InsertNextTuple((85, 8, 2))

        b = vtkFloatArray()
        b.SetVoidArray(a, 6, True)
        self.assertEqual(buffer_shared(a, b), True)

        c = vtkFloatArray()
        c.DeepCopy(a)
        self.assertEqual(buffer_shared(a, c), False)

        m = memoryview(a)
        self.assertEqual(buffer_shared(a, m), True)

if __name__ == "__main__":
    Testing.main([(TestBuffer, 'test')])
