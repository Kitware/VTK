"""vtkDeviceDataArray over Level Zero memory: VTK holding it without Viskores.

The memory comes from the Level Zero loader, called through ctypes in VTK's
own Level Zero context (``dlpack_support.level_zero_handles()``), so the test
needs no GPU library beyond the loader -- the same thing VTK needs -- and skips
where there is no Level Zero GPU.
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

# ze_api.h's values (API 1.9).
ZE_STRUCTURE_TYPE_CONTEXT_DESC = 0xD
ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC = 0xE
ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC = 0x15
ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS = 1


class ContextDesc(ctypes.Structure):
    _fields_ = [("stype", ctypes.c_uint32), ("pNext", ctypes.c_void_p),
                ("flags", ctypes.c_uint32)]


class DeviceMemAllocDesc(ctypes.Structure):
    _fields_ = [("stype", ctypes.c_uint32), ("pNext", ctypes.c_void_p),
                ("flags", ctypes.c_uint32), ("ordinal", ctypes.c_uint32)]


class CommandQueueDesc(ctypes.Structure):
    _fields_ = [("stype", ctypes.c_uint32), ("pNext", ctypes.c_void_p),
                ("ordinal", ctypes.c_uint32), ("index", ctypes.c_uint32),
                ("flags", ctypes.c_uint32), ("mode", ctypes.c_uint32),
                ("priority", ctypes.c_uint32)]


def load_level_zero():
    """The Level Zero loader, or None."""
    name = "ze_loader.dll" if sys.platform == "win32" else "libze_loader.so.1"
    try:
        ze = ctypes.CDLL(name)
    except OSError:
        return None
    P = ctypes.c_void_p
    for name, args in (
            ("zeContextCreate", [P, ctypes.POINTER(ContextDesc), ctypes.POINTER(P)]),
            ("zeMemAllocDevice", [P, ctypes.POINTER(DeviceMemAllocDesc), ctypes.c_size_t,
                                  ctypes.c_size_t, P, ctypes.POINTER(P)]),
            ("zeMemFree", [P, P]),
            ("zeCommandListCreateImmediate", [P, P, ctypes.POINTER(CommandQueueDesc),
                                              ctypes.POINTER(P)]),
            ("zeCommandListAppendMemoryCopy", [P, P, P, ctypes.c_size_t, P, ctypes.c_uint32,
                                               P])):
        getattr(ze, name).argtypes = args
        getattr(ze, name).restype = ctypes.c_int
    return ze


ZE = load_level_zero()
HANDLES = dlpack_support.level_zero_handles() if ZE else None
RELEASE = ctypes.CFUNCTYPE(None, ctypes.c_void_p)


class Context:
    """A Level Zero context and a synchronous list to copy with in it."""

    def __init__(self, driver, device, context):
        self.driver, self.device, self.context = driver, device, context
        self.list = ctypes.c_void_p()
        desc = CommandQueueDesc(stype=ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC,
                                mode=ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS)
        assert ZE.zeCommandListCreateImmediate(
            context, device, ctypes.byref(desc), ctypes.byref(self.list)) == 0

    def copy(self, destination, source, nbytes):
        assert ZE.zeCommandListAppendMemoryCopy(
            self.list, destination, source, nbytes, None, 0, None) == 0


VTK_CONTEXT = Context(**HANDLES) if HANDLES else None


class DeviceBuffer:
    """Device memory holding *values*, allocated in *context* (VTK's by default)
    and freed by the release callback a descriptor carries, so the test sees
    when VTK lets go of it."""

    def __init__(self, values, context=None):
        self.context = context or VTK_CONTEXT
        self.values = numpy.ascontiguousarray(values)
        self.address = ctypes.c_void_p()
        desc = DeviceMemAllocDesc(stype=ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC)
        assert ZE.zeMemAllocDevice(self.context.context, ctypes.byref(desc),
                                   self.values.nbytes, 64, self.context.device,
                                   ctypes.byref(self.address)) == 0
        self.write(self.values)
        self.released = 0
        self._release = RELEASE(self._free)    # kept: VTK calls it later

    def write(self, values):
        values = numpy.ascontiguousarray(values, dtype=self.values.dtype)
        self.context.copy(self.address, values.ctypes.data, values.nbytes)

    def read(self):
        out = numpy.empty_like(self.values)
        self.context.copy(out.ctypes.data, self.address, out.nbytes)
        return out

    def _free(self, context):
        self.released += 1
        ZE.zeMemFree(self.context.context, self.address)

    def descriptor(self):
        descriptor = vtkMemoryDescriptor()
        descriptor.Set(self.address.value, self.values.nbytes, "level_zero", "data")
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


def viskores():
    """Whether this VTK has Viskores' arrays."""
    try:
        import vtkmodules.vtkAcceleratorsVTKmCore  # noqa: F401
    except ImportError:
        return False
    return True


@unittest.skipUnless(HANDLES, "needs the Level Zero loader and a GPU")
class TestDeviceDataArrayLevelZero(unittest.TestCase):

    def setUp(self):
        self.values = numpy.arange(3000, dtype=numpy.float32).reshape(1000, 3)
        self.buffer = DeviceBuffer(self.values)

    def testTheVendorDriverIsFound(self):
        self.assertTrue(vtkDeviceArrayFactory.IsMemorySpaceAvailable("level_zero"))
        self.assertFalse(vtkDeviceArrayFactory.IsMemorySpaceAvailable("host"))
        self.assertFalse(vtkDeviceArrayFactory.IsMemorySpaceAvailable("metal"))

    def testVTKHasAContextOfItsOwn(self):
        # Without Viskores the context is VTK's own; with it, Viskores' (adopted).
        self.assertTrue(vtkDeviceArrayFactory.HasLevelZeroContext())
        self.assertEqual(HANDLES, {
            "driver": vtkDeviceArrayFactory.GetLevelZeroDriverHandle(),
            "device": vtkDeviceArrayFactory.GetLevelZeroDeviceHandle(),
            "context": vtkDeviceArrayFactory.GetLevelZeroContextHandle()})
        self.assertTrue(all(HANDLES.values()))
        if not viskores():
            self.assertEqual(dlpack_support.level_zero_handles(), HANDLES)

    def testAGivenContextReplacesIt(self):
        made = ctypes.c_void_p()
        desc = ContextDesc(stype=ZE_STRUCTURE_TYPE_CONTEXT_DESC)
        self.assertEqual(ZE.zeContextCreate(HANDLES["driver"], ctypes.byref(desc),
                                            ctypes.byref(made)), 0)
        given = dict(HANDLES, context=made.value)
        try:
            vtkDeviceArrayFactory.SetLevelZeroContext(
                given["driver"], given["device"], given["context"])
            self.assertEqual(dlpack_support.level_zero_handles(), given)
            # Memory of the given context is held and read there.
            other = DeviceBuffer(self.values * 2, Context(**given))
            array = other.array()
            self.assertEqual(array.GetTuple3(999), (5994.0, 5996.0, 5998.0))
            del array
            gc.collect()
            self.assertEqual(other.released, 1)
        finally:
            vtkDeviceArrayFactory.SetLevelZeroContext(
                HANDLES["driver"], HANDLES["device"], HANDLES["context"])
        self.assertEqual(dlpack_support.level_zero_handles(), HANDLES)
        # The made context is not destroyed: VTK may still hold the list it
        # copied with there.

    def testHostReadsSeeTheDeviceValues(self):
        array = self.buffer.array()
        self.assertEqual(array.GetMemorySpace(), vtkDataArray.LevelZeroDeviceMemory)
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
        self.assertEqual(descriptor.GetMemorySpace(), "level_zero")
        self.assertEqual(descriptor.GetRole(), "data")
        self.assertEqual(descriptor.GetSizeInBytes(), self.values.nbytes)
        self.assertEqual(array.GetDeviceVoidPointer(0) is not None, True)
        exported = dlpack_support.DLPackArray(array)
        self.assertEqual(exported.__dlpack_device__(), (dlpack_support.kDLOneAPI, 0))

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

    def testLargeReadsSeeTheDeviceValues(self):
        # 64 MB, past the sizes at which a copy from pageable memory behaves
        # differently on this driver (see the Kokkos/SYCL host-to-device copy).
        values = numpy.arange(16 * 1024 * 1024, dtype=numpy.float32).reshape(-1, 4)
        array = DeviceBuffer(values).array()
        for _ in range(3):
            numpy.testing.assert_array_equal(numpy_support.vtk_to_numpy(array), values)
            array.Modified()

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

    def testHostMemoryLabelledLevelZeroIsRefused(self):
        host = numpy.zeros(4, numpy.float32)
        descriptor = vtkMemoryDescriptor()
        descriptor.Set(host.ctypes.data, host.nbytes, "level_zero", "data")
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
    Testing.main([(TestDeviceDataArrayLevelZero, "test")])
