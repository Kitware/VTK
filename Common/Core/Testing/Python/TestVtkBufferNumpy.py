"""Test buffer protocol for vtkBuffer template class.

vtkBuffer is a template class used internally by vtkDataArray subclasses.
This test verifies that the Python buffer protocol is correctly implemented
for vtkBuffer, enabling zero-copy interoperability with NumPy.
"""

import sys
import struct
from vtkmodules.vtkCommonCore import (
    VTK_SIZEOF_LONG,
    vtkBuffer,
)
from vtkmodules.test import Testing

try:
    import numpy
except ImportError:
    print("This test requires numpy!")
    Testing.skip()


class TestVtkBufferNumpy(Testing.vtkTest):
    """Test vtkBuffer Python buffer protocol with NumPy."""

    def testBufferFloat64(self):
        """Test vtkBuffer with float64 (double) type."""
        buf = vtkBuffer['float64']()
        buf.Allocate(10)

        arr = numpy.asarray(buf)
        self.assertEqual(arr.shape, (10,))
        self.assertEqual(arr.dtype, numpy.float64)

        # Write data through numpy
        arr[:] = numpy.arange(10, dtype=numpy.float64)

        # Verify data persists
        arr2 = numpy.asarray(buf)
        self.assertTrue(numpy.allclose(arr2, numpy.arange(10)))

    def testBufferFloat32(self):
        """Test vtkBuffer with float32 (float) type."""
        buf = vtkBuffer['float32']()
        buf.Allocate(5)

        arr = numpy.asarray(buf)
        self.assertEqual(arr.shape, (5,))
        self.assertEqual(arr.dtype, numpy.float32)

        arr[:] = [1.5, 2.5, 3.5, 4.5, 5.5]
        arr2 = numpy.asarray(buf)
        self.assertTrue(numpy.allclose(arr2, [1.5, 2.5, 3.5, 4.5, 5.5]))

    def testBufferInt32(self):
        """Test vtkBuffer with int32 type."""
        buf = vtkBuffer['int32']()
        buf.Allocate(8)

        arr = numpy.asarray(buf)
        self.assertEqual(arr.shape, (8,))
        self.assertEqual(arr.dtype, numpy.int32)

        arr[:] = [1, 2, 3, 4, 5, 6, 7, 8]
        arr2 = numpy.asarray(buf)
        self.assertTrue(numpy.array_equal(arr2, [1, 2, 3, 4, 5, 6, 7, 8]))

    def testBufferUint8(self):
        """Test vtkBuffer with uint8 (unsigned char) type."""
        buf = vtkBuffer['uint8']()
        buf.Allocate(256)

        arr = numpy.asarray(buf)
        self.assertEqual(arr.shape, (256,))
        self.assertEqual(arr.dtype, numpy.uint8)

        arr[:] = numpy.arange(256, dtype=numpy.uint8)
        arr2 = numpy.asarray(buf)
        self.assertTrue(numpy.array_equal(arr2, numpy.arange(256, dtype=numpy.uint8)))

    def testBufferInt64(self):
        """Test vtkBuffer with int64 type."""
        buf = vtkBuffer['int64']()
        buf.Allocate(4)

        arr = numpy.asarray(buf)
        self.assertEqual(arr.shape, (4,))
        self.assertEqual(arr.dtype, numpy.int64)

        # Test with large values
        large_vals = [2**40, 2**50, 2**60, -2**62]
        arr[:] = large_vals
        arr2 = numpy.asarray(buf)
        self.assertTrue(numpy.array_equal(arr2, large_vals))

    def testBufferInt16(self):
        """Test vtkBuffer with int16 (short) type."""
        buf = vtkBuffer['int16']()
        buf.Allocate(3)

        arr = numpy.asarray(buf)
        self.assertEqual(arr.shape, (3,))
        self.assertEqual(arr.dtype, numpy.int16)

        arr[:] = [-32768, 0, 32767]
        arr2 = numpy.asarray(buf)
        self.assertTrue(numpy.array_equal(arr2, [-32768, 0, 32767]))

    def testBufferUint16(self):
        """Test vtkBuffer with uint16 (unsigned short) type."""
        buf = vtkBuffer['uint16']()
        buf.Allocate(3)

        arr = numpy.asarray(buf)
        self.assertEqual(arr.shape, (3,))
        self.assertEqual(arr.dtype, numpy.uint16)

        arr[:] = [0, 32768, 65535]
        arr2 = numpy.asarray(buf)
        self.assertTrue(numpy.array_equal(arr2, [0, 32768, 65535]))

    def testZeroCopySemantics(self):
        """Test that numpy arrays share memory with vtkBuffer (zero-copy)."""
        buf = vtkBuffer['float64']()
        buf.Allocate(5)

        arr1 = numpy.asarray(buf)
        arr2 = numpy.asarray(buf)

        # Verify they share memory
        self.assertTrue(numpy.shares_memory(arr1, arr2))

        # Modify through first array
        arr1[:] = [10.0, 20.0, 30.0, 40.0, 50.0]

        # Second array should see the changes
        self.assertTrue(numpy.array_equal(arr2, [10.0, 20.0, 30.0, 40.0, 50.0]))

        # Modify through second array
        arr2[2] = 999.0

        # First array should see the change
        self.assertEqual(arr1[2], 999.0)

    def testMemoryView(self):
        """Test that memoryview works with vtkBuffer."""
        buf = vtkBuffer['int32']()
        buf.Allocate(4)

        # Get memoryview
        m = memoryview(buf)
        self.assertEqual(m.ndim, 1)
        self.assertEqual(m.shape, (4,))
        self.assertEqual(m.itemsize, 4)
        self.assertEqual(m.format, 'i')

        # Write through numpy, read through memoryview
        arr = numpy.asarray(buf)
        arr[:] = [100, 200, 300, 400]
        vals = struct.unpack('4i', m.tobytes())
        self.assertEqual(vals, (100, 200, 300, 400))

    def testEmptyBuffer(self):
        """Test vtkBuffer with zero size."""
        buf = vtkBuffer['float64']()
        buf.Allocate(0)

        arr = numpy.asarray(buf)
        self.assertEqual(arr.shape, (0,))
        self.assertEqual(len(arr), 0)

    def testReallocate(self):
        """Test that buffer protocol works after reallocation."""
        buf = vtkBuffer['float64']()
        buf.Allocate(5)

        arr = numpy.asarray(buf)
        arr[:] = [1.0, 2.0, 3.0, 4.0, 5.0]

        # Reallocate to larger size (preserves data)
        buf.Reallocate(10)

        arr2 = numpy.asarray(buf)
        self.assertEqual(arr2.shape, (10,))
        # First 5 elements should be preserved
        self.assertTrue(numpy.allclose(arr2[:5], [1.0, 2.0, 3.0, 4.0, 5.0]))

    def testAllScalarTypes(self):
        """Test all supported scalar types have correct numpy dtype mapping."""
        type_map = {
            'float32': numpy.float32,
            'float64': numpy.float64,
            'int8': numpy.int8,
            'uint8': numpy.uint8,
            'int16': numpy.int16,
            'uint16': numpy.uint16,
            'int32': numpy.int32,
            'uint32': numpy.uint32,
            'int64': numpy.int64,
            'uint64': numpy.uint64,
        }

        for vtk_type, np_dtype in type_map.items():
            buf = vtkBuffer[vtk_type]()
            buf.Allocate(3)
            arr = numpy.asarray(buf)
            self.assertEqual(arr.dtype, np_dtype,
                f"Type mismatch for {vtk_type}: expected {np_dtype}, got {arr.dtype}")


if __name__ == "__main__":
    Testing.main([(TestVtkBufferNumpy, 'test')])
