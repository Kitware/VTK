"""Test if the NumPy array support for VTK data arrays works correctly.
The test requires that numpy (http://numpy.scipy.org) work.

Run this test like so:
 $ vtkpython TestNumpySupport.py
or
 $ python TestNumpySupport.py

"""

import sys
from vtkmodules.vtkCommonCore import (
    VTK_INT,
    vtkBitArray,
    vtkLongArray,
)
from vtkmodules.test import Testing
try:
    import numpy
except ImportError:
    print("This test requires numpy!")
    Testing.skip()

from vtkmodules.util.numpy_support import numpy_to_vtk,vtk_to_numpy
import vtkmodules.numpy_interface.dataset_adapter as dsa


class TestNumpySupport(Testing.vtkTest):
    def _check_arrays(self, arr, vtk_arr):
        """Private method to check array.
        """
        self.assertEqual(vtk_arr.GetNumberOfTuples(), len(arr))
        if len(arr.shape) == 2:
            dim1 = arr.shape[1]
            self.assertEqual(vtk_arr.GetNumberOfComponents(), dim1)
            for i in range(len(arr)):
                if dim1 in [1,2,3,4,9]:
                    res = getattr(vtk_arr, 'GetTuple%s'%dim1)(i)
                    self.assertEqual(numpy.sum(res - arr[i]), 0)
                else:
                    res = [vtk_arr.GetComponent(i, j) for j in range(dim1)]
                    self.assertEqual(numpy.sum(res - arr[i]), 0)
        else:
            for i in range(len(arr)):
                self.assertEqual(vtk_arr.GetTuple1(i), arr[i])

    def testNumpy2VTK(self):
        """Test Numeric array to VTK array conversion and vice-versa."""
        # Put all the test arrays here.
        t_z = []

        # Test the different types of arrays.
        t_z.append(numpy.array([-128, 0, 127], numpy.int8))
        t_z.append(numpy.array([-32768, 0, 32767], numpy.int16))
        t_z.append(numpy.array([-2147483648, 0, 2147483647], numpy.int32))
        t_z.append(numpy.array([0, 255], numpy.uint8))
        t_z.append(numpy.array([0, 65535], numpy.uint16))
        t_z.append(numpy.array([0, 4294967295], numpy.uint32))
        t_z.append(numpy.array([-1.0e38, 0, 1.0e38], 'f'))
        t_z.append(numpy.array([-1.0e299, 0, 1.0e299], 'd'))

        # Check multi-component arrays.
        t_z.append(numpy.array([[1], [2], [300]], 'd'))
        t_z.append(numpy.array([[1, 20], [300, 4000]], 'd'))
        t_z.append(numpy.array([[1, 2, 3], [4, 5, 6]], 'f'))
        t_z.append(numpy.array([[1, 2, 3],[4, 5, 6]], 'd'))
        t_z.append(numpy.array([[1, 2, 3, 400],[4, 5, 6, 700]],
                                 'd'))
        t_z.append(numpy.array([range(9),range(10,19)], 'f'))

        # Test if arrays with number of components not in [1,2,3,4,9] work.
        t_z.append(numpy.array([[1, 2, 3, 400, 5000],
                                  [4, 5, 6, 700, 8000]], 'd'))
        t_z.append(numpy.array([range(10), range(10,20)], 'd'))

        for z in t_z:
            vtk_arr = numpy_to_vtk(z)
            # Test for memory leaks.
            self.assertEqual(vtk_arr.GetReferenceCount(), 1)
            self._check_arrays(z, vtk_arr)
            z1 = vtk_to_numpy(vtk_arr)
            if len(z.shape) == 1:
                self.assertEqual(len(z1.shape), 1)
            self.assertEqual(sum(numpy.ravel(z) - numpy.ravel(z1)), 0)

        vtk_arr = vtkBitArray()
        bits = [0, 1, 0, 1, 1, 1, 0, 0, 1, 0]
        for bit in bits:
            vtk_arr.InsertNextValue(bit)

        res = vtk_to_numpy(vtk_arr)
        self._check_arrays(res, vtk_arr)
        vtk_arr = numpy_to_vtk(res)
        self._check_arrays(res, vtk_arr)

    def testNumpyView(self):
        "Test if the numpy and VTK array share the same data."
        # ----------------------------------------
        # Test if the array is copied or not.
        a = numpy.array([[1, 2, 3],[4, 5, 6]], 'd')
        vtk_arr = numpy_to_vtk(a)
        # Change the numpy array and see if the changes are
        # reflected in the VTK array.
        a[0] = [10.0, 20.0, 30.0]
        self.assertEqual(vtk_arr.GetTuple3(0), (10., 20., 30.))

    def testNumpyConversion(self):
        "Test that converting data copies data properly."
        # ----------------------------------------
        # Test if the array is copied or not.
        a = numpy.array([[1, 2, 3],[4, 5, 6]], 'd')
        vtk_arr = numpy_to_vtk(a, 0, VTK_INT)
        # Change the numpy array and see if the changes are
        # reflected in the VTK array.
        a[0] = [10.0, 20.0, 30.0]
        self.assertEqual(vtk_arr.GetTuple3(0), (1., 2., 3.))

    def testNonContiguousArray(self):
        "Test if the non contiguous array are supported"
        a = numpy.array(range(1, 19), 'd')
        a.shape = (3, 6)
        x = a[::2, ::2]
        vtk_array = numpy_to_vtk(x)
        self.assertEqual(vtk_array.GetTuple3(0), (1., 3., 5.))
        self.assertEqual(vtk_array.GetTuple3(1), (13., 15., 17.))

    def testNumpyReferenceWhenDelete(self):
        "Test if the vtk array keeps the numpy reference in memory"
        np_array = numpy.array([[1., 3., 5.], [13., 15., 17.]], 'd')
        vtk_array = numpy_to_vtk(np_array)
        del np_array
        np_array = numpy.array([])

        import gc
        gc.collect()

        self.assertEqual(vtk_array.GetTuple3(0), (1., 3., 5.))
        self.assertEqual(vtk_array.GetTuple3(1), (13., 15., 17.))

    def testNumpyReduce(self):
        "Test that reducing methods return scalars."
        vtk_array = vtkLongArray()
        for i in range(0, 10):
            vtk_array.InsertNextValue(i)

        numpy_vtk_array = dsa.vtkDataArrayToVTKArray(vtk_array)
        s = numpy_vtk_array.sum()
        self.assertEqual(s, 45)
        self.assertTrue(isinstance(s, numpy.signedinteger))

        m = numpy_vtk_array.mean()
        self.assertEqual(m, 4.5)
        self.assertTrue(isinstance(m, numpy.floating))

if __name__ == "__main__":
    Testing.main([(TestNumpySupport, 'test')])
