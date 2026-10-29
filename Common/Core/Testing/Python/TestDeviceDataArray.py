"""vtkDeviceDataArray: VTK holding GPU memory without Viskores.

The memory comes from the CUDA driver, called through ctypes, so the test
needs no GPU library beyond the driver -- the same thing VTK needs -- and
skips where there is none.
"""

import ctypes
import gc
import sys
import unittest

from vtkmodules.vtkCommonCore import (
    VTK_FLOAT,
    vtkDataArray,
    vtkDeviceArrayFactory,
    vtkFloatArray,
    vtkMemoryDescriptor,
    vtkObject,
)
from vtkmodules.test import Testing

try:
    import numpy
except ImportError:
    print("This test requires numpy!")
    Testing.skip()

from vtkmodules.util import dlpack_support, numpy_support


def load_cuda():
    """The CUDA driver with a current context, or None."""
    name = "nvcuda.dll" if sys.platform == "win32" else "libcuda.so.1"
    try:
        cuda = ctypes.CDLL(name)
    except OSError:
        return None
    device, context = ctypes.c_int(), ctypes.c_void_p()
    if (cuda.cuInit(0) != 0 or cuda.cuDeviceGet(ctypes.byref(device), 0) != 0
            or cuda.cuDevicePrimaryCtxRetain(ctypes.byref(context), device) != 0
            or cuda.cuCtxSetCurrent(context) != 0):
        return None
    for name, args in (("cuMemAlloc_v2", [ctypes.POINTER(ctypes.c_uint64), ctypes.c_size_t]),
                       ("cuMemFree_v2", [ctypes.c_uint64]),
                       ("cuMemcpyHtoD_v2", [ctypes.c_uint64, ctypes.c_void_p, ctypes.c_size_t]),
                       ("cuMemcpyDtoH_v2", [ctypes.c_void_p, ctypes.c_uint64, ctypes.c_size_t])):
        getattr(cuda, name).argtypes = args
    return cuda


CUDA = load_cuda()
RELEASE = ctypes.CFUNCTYPE(None, ctypes.c_void_p)


class DeviceBuffer:
    """Device memory holding *values*, freed by the release callback a
    descriptor carries, so the test sees when VTK lets go of it."""

    def __init__(self, values):
        self.values = numpy.ascontiguousarray(values)
        self.address = ctypes.c_uint64()
        assert CUDA.cuMemAlloc_v2(ctypes.byref(self.address), self.values.nbytes) == 0
        self.write(self.values)
        self.released = 0
        self._release = RELEASE(self._free)    # kept: VTK calls it later

    def write(self, values):
        values = numpy.ascontiguousarray(values, dtype=self.values.dtype)
        assert CUDA.cuMemcpyHtoD_v2(self.address.value, values.ctypes.data, values.nbytes) == 0

    def read(self):
        out = numpy.empty_like(self.values)
        assert CUDA.cuMemcpyDtoH_v2(out.ctypes.data, self.address.value, out.nbytes) == 0
        return out

    def _free(self, context):
        self.released += 1
        CUDA.cuMemFree_v2(self.address.value)

    def descriptor(self):
        descriptor = vtkMemoryDescriptor()
        descriptor.Set(self.address.value, self.values.nbytes, "cuda", "data")
        descriptor.SetReleaseAddress(
            ctypes.cast(self._release, ctypes.c_void_p).value, 0)
        return descriptor

    def array(self, vtk_type=VTK_FLOAT):
        tuples, components = (self.values.shape + (1,))[:2]
        factory = vtkDeviceArrayFactory()
        factory.SetNumberOfTuples(tuples)
        factory.SetNumberOfComponents(components)
        factory.SetDataType(vtk_type)
        factory.AddBuffer(self.descriptor())
        return factory.CreateArray()


class Errors(list):
    """The errors an object reports, caught rather than printed. (Turning the
    global display off would not do: it drops the event too.)"""

    CallDataType = "string0"

    def __call__(self, caller, event, message):
        self.append(message)

    def saw(self, text):
        return any(text in message for message in self)


def quietly(array, call):
    """Call *call*, and return the errors *array* reported."""
    errors = Errors()
    array.AddObserver("ErrorEvent", errors)
    call()
    return errors


@unittest.skipUnless(CUDA, "needs the CUDA driver and a GPU")
class TestDeviceDataArray(unittest.TestCase):

    def setUp(self):
        self.values = numpy.arange(3000, dtype=numpy.float32).reshape(1000, 3)
        self.buffer = DeviceBuffer(self.values)

    def testTheVendorDriverIsFound(self):
        self.assertTrue(vtkDeviceArrayFactory.IsMemorySpaceAvailable("cuda"))
        self.assertFalse(vtkDeviceArrayFactory.IsMemorySpaceAvailable("host"))
        self.assertFalse(vtkDeviceArrayFactory.IsMemorySpaceAvailable("metal"))

    def testHostReadsSeeTheDeviceValues(self):
        array = self.buffer.array()
        self.assertEqual(array.GetMemorySpace(), vtkDataArray.CudaDeviceMemory)
        self.assertEqual((array.GetNumberOfTuples(), array.GetNumberOfComponents()), (1000, 3))
        self.assertEqual(array.GetComponent(2, 1), 7.0)
        self.assertEqual(array.GetTuple3(999), (2997.0, 2998.0, 2999.0))
        self.assertEqual(array.GetRange(0), (0.0, 2997.0))
        numpy.testing.assert_array_equal(numpy_support.vtk_to_numpy(array), self.values)

    def testDescriptorsGiveTheDeviceMemoryBack(self):
        array = self.buffer.array()
        descriptors = array.NewMemoryDescriptors()
        self.assertEqual(descriptors.GetNumberOfItems(), 1)
        descriptor = descriptors.GetItemAsObject(0)
        self.assertEqual(descriptor.GetPointer(), self.buffer.address.value)
        self.assertEqual(descriptor.GetMemorySpace(), "cuda")
        self.assertEqual(descriptor.GetRole(), "data")
        self.assertEqual(descriptor.GetSizeInBytes(), self.values.nbytes)
        self.assertEqual(array.GetDeviceVoidPointer(0) is not None, True)
        exported = dlpack_support.DLPackArray(array)
        self.assertEqual(exported.__dlpack_device__(), (dlpack_support.kDLCUDA, 0))

    def testHostWritesAreRefused(self):
        array = self.buffer.array()
        errors = quietly(array, lambda: array.SetComponent(0, 0, 42.0))
        self.assertTrue(errors.saw("read-only on the host"), errors)
        self.assertEqual(array.GetComponent(0, 0), 0.0)
        numpy.testing.assert_array_equal(self.buffer.read(), self.values)

    def testGrowthIsRefused(self):
        array = self.buffer.array()
        errors = quietly(array, lambda: array.InsertNextTuple3(1.0, 2.0, 3.0))
        self.assertTrue(errors.saw("cannot grow"), errors)
        self.assertEqual(array.GetNumberOfTuples(), 1000)

    def testShrinkingKeepsTheMemory(self):
        array = self.buffer.array()
        array.SetNumberOfTuples(10)
        self.assertEqual(array.GetNumberOfTuples(), 10)
        self.assertEqual(array.GetTuple3(9), (27.0, 28.0, 29.0))
        array.SetNumberOfTuples(1000)
        self.assertEqual(array.GetTuple3(999), (2997.0, 2998.0, 2999.0))

    def testNewInstanceIsAWritableHostArray(self):
        array = self.buffer.array()
        copy = array.NewInstance()
        self.assertEqual(copy.GetMemorySpace(), vtkDataArray.HostMemory)
        copy.DeepCopy(array)
        copy.SetComponent(0, 0, 42.0)
        copy.InsertNextTuple3(1.0, 2.0, 3.0)
        self.assertEqual(copy.GetNumberOfTuples(), 1001)
        self.assertEqual(copy.GetComponent(1, 0), 3.0)
        host = vtkFloatArray()
        host.DeepCopy(array)
        numpy.testing.assert_array_equal(numpy_support.vtk_to_numpy(host), self.values)

    def testModifiedReadsTheDeviceAgain(self):
        array = self.buffer.array()
        self.assertEqual(array.GetComponent(0, 0), 0.0)
        self.buffer.write(self.values + 1)
        self.assertEqual(array.GetComponent(0, 0), 0.0)   # the host copy, until told
        array.Modified()
        self.assertEqual(array.GetComponent(0, 0), 1.0)

    def testShallowCopySharesTheMemory(self):
        array = self.buffer.array()
        other = DeviceBuffer(numpy.zeros((2, 3), numpy.float32)).array()
        other.ShallowCopy(array)
        self.assertEqual(other.GetNumberOfTuples(), 1000)
        self.assertEqual(other.GetComponent(2, 1), 7.0)
        errors = quietly(other, lambda: other.ShallowCopy(vtkFloatArray()))
        self.assertTrue(errors.saw("can only share"), errors)

    def testTheMemoryIsReleasedWithTheLastHolder(self):
        array = self.buffer.array()
        exported = dlpack_support.vtk_to_dlpack(array)
        del array
        gc.collect()
        self.assertEqual(self.buffer.released, 0)    # the export still holds it
        del exported
        gc.collect()
        self.assertEqual(self.buffer.released, 1)

    def testHostMemoryLabelledCudaIsRefused(self):
        host = numpy.zeros(4, numpy.float32)
        descriptor = vtkMemoryDescriptor()
        descriptor.Set(host.ctypes.data, host.nbytes, "cuda", "data")
        factory = vtkDeviceArrayFactory()
        factory.SetNumberOfTuples(4)
        factory.SetDataType(VTK_FLOAT)
        factory.AddBuffer(descriptor)
        vtkObject.GlobalWarningDisplayOff()
        try:
            self.assertIsNone(factory.CreateArray())
        finally:
            vtkObject.GlobalWarningDisplayOn()

    def testTooSmallABufferIsRefused(self):
        factory = vtkDeviceArrayFactory()
        factory.SetNumberOfTuples(2000)
        factory.SetNumberOfComponents(3)
        factory.SetDataType(VTK_FLOAT)
        factory.AddBuffer(self.buffer.descriptor())
        vtkObject.GlobalWarningDisplayOff()
        try:
            self.assertIsNone(factory.CreateArray())
        finally:
            vtkObject.GlobalWarningDisplayOn()


if __name__ == "__main__":
    Testing.main([(TestDeviceDataArray, "test")])
