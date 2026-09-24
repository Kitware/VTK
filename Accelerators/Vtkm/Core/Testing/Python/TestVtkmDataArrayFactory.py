"""Wrapping external memory as a vtkmDataArray.

The factory takes a pointer that VTK did not allocate and hands back an
array Viskores filters can run on, without copying. What makes that safe
rather than merely convenient is the release: the descriptor's hold is
transferred into the array, and the producer is told when Viskores is
finished.

That handoff is the part worth testing directly, because both ways of
getting it wrong are silent. Release too early and filters read freed
memory -- usually returning plausible numbers. Never release and every
array leaks, along with whatever the producer was holding for it, which
on a device is far less forgiving than on the host.

The memory here is host memory, so these run anywhere Viskores is built.
The device spaces need hardware and a matching device adapter.
"""

import contextlib
import ctypes
import gc

import numpy

from vtkmodules.vtkAcceleratorsVTKmCore import vtkmDataArrayFactory
from vtkmodules.vtkCommonCore import vtkMemoryDescriptor, vtkObject
from vtkmodules.test import Testing

VTK_FLOAT = 10


@contextlib.contextmanager
def refusal_is_expected():
    """Keep a deliberate vtkErrorMacro off the console.

    ctest fails any VTK Python test whose output matches "ERR|"
    (vtkModuleTesting.cmake), so a test that drives a refusal path on
    purpose has to silence the message it is asking for -- otherwise
    proving the factory refuses bad memory is what makes the test fail.

    Only the printing is suppressed, and only for the call it wraps. That
    the factory actually refused is still asserted by the caller.
    """
    vtkObject.GlobalWarningDisplayOff()
    try:
        yield
    finally:
        vtkObject.GlobalWarningDisplayOn()

# Released contexts land here. Module scope, because the callback must
# outlive every descriptor that points at it.
released = []


@ctypes.CFUNCTYPE(None, ctypes.c_void_p)
def record_release(context):
    released.append(context)


RELEASE_ADDRESS = ctypes.cast(record_release, ctypes.c_void_p).value


def make_descriptor(source, tag=None):
    """Describe *source*'s memory, optionally with a release callback."""
    descriptor = vtkMemoryDescriptor()
    descriptor.Set(source.ctypes.data, source.nbytes, "host", "data")
    if tag is not None:
        descriptor.SetReleaseAddress(RELEASE_ADDRESS, tag)
    return descriptor


def make_array(source, n_components=1, tag=None):
    descriptor = make_descriptor(source, tag)
    factory = vtkmDataArrayFactory()
    factory.SetNumberOfTuples(source.size // n_components)
    factory.SetNumberOfComponents(n_components)
    factory.SetDataType(VTK_FLOAT)
    factory.AddBuffer(descriptor)
    return factory.CreateArray()


class TestVtkmDataArrayFactory(Testing.vtkTest):
    def setUp(self):
        del released[:]
        gc.collect()

    def testWrapsExternalMemory(self):
        source = numpy.arange(12, dtype=numpy.float32)
        array = make_array(source, n_components=3)

        self.assertEqual(array.GetNumberOfTuples(), 4)
        self.assertEqual(array.GetNumberOfComponents(), 3)
        values = [array.GetComponent(t, c) for t in range(4) for c in range(3)]
        self.assertEqual(values, list(range(12)))

    def testSeesWritesThroughTheSource(self):
        """The consequence of not copying."""
        source = numpy.zeros(4, dtype=numpy.float32)
        array = make_array(source)
        source[2] = 42.0
        self.assertEqual(array.GetComponent(2, 0), 42.0)

    def testTheHoldMovesOffTheDescriptor(self):
        """Two owners of one release would call it twice."""
        source = numpy.arange(4, dtype=numpy.float32)
        descriptor = make_descriptor(source, tag=1)
        self.assertTrue(descriptor.HasOwnership())

        factory = vtkmDataArrayFactory()
        factory.SetNumberOfTuples(4)
        factory.SetNumberOfComponents(1)
        factory.SetDataType(VTK_FLOAT)
        factory.AddBuffer(descriptor)
        array = factory.CreateArray()

        self.assertIsNotNone(array)
        self.assertFalse(descriptor.HasOwnership())
        self.assertEqual(released, [])

    def testReleaseRunsWhenTheArrayDies(self):
        """The whole point: the producer is told when Viskores is done."""
        source = numpy.arange(4, dtype=numpy.float32)
        array = make_array(source, tag=7)
        self.assertEqual(released, [])

        del array
        gc.collect()
        self.assertEqual(released, [7])

    def testReleaseRunsExactlyOnce(self):
        # Tags start at 1: a context of 0 round-trips through c_void_p as
        # None, so it is indistinguishable from "no context".
        for tag in range(1, 21):
            array = make_array(numpy.arange(4, dtype=numpy.float32), tag=tag)
            del array
        gc.collect()
        self.assertEqual(sorted(released), list(range(1, 21)))

    def testDescriptorsSurviveTheRoundTrip(self):
        """Out through the same interface the array came in by."""
        source = numpy.arange(12, dtype=numpy.float32)
        array = make_array(source, n_components=3, tag=3)

        descriptors = array.NewMemoryDescriptors()
        self.assertEqual(descriptors.GetNumberOfItems(), 1)
        descriptor = descriptors.GetItemAsObject(0)
        self.assertEqual(descriptor.GetMemorySpace(), "host")
        self.assertEqual(descriptor.GetPointer(), source.ctypes.data)
        self.assertEqual(descriptor.GetSizeInBytes(), source.nbytes)

        del array, descriptors, descriptor
        gc.collect()
        self.assertEqual(released, [3])

    def testExportsThroughDLPack(self):
        from vtkmodules.util.dlpack_support import DLPackArray

        source = numpy.arange(12, dtype=numpy.float32)
        array = make_array(source, n_components=3)
        view = numpy.from_dlpack(DLPackArray(array))

        self.assertEqual(view.shape, (4, 3))
        self.assertEqual(
            view.__array_interface__["data"][0], source.ctypes.data
        )

    def testNullPointerIsRefused(self):
        descriptor = vtkMemoryDescriptor()
        descriptor.Set(0, 16, "host", "data")

        factory = vtkmDataArrayFactory()
        factory.SetNumberOfTuples(4)
        factory.SetNumberOfComponents(1)
        factory.SetDataType(VTK_FLOAT)
        factory.AddBuffer(descriptor)

        with refusal_is_expected():
            self.assertIsNone(factory.CreateArray())

    def testUnreadableMemorySpaceIsRefused(self):
        """Treating an unreadable space as host hands out a bad pointer.

        "level_zero" has no Viskores device adapter, and "cuda" has none on
        a build without CUDA. Falling back to host in either case is how a
        misconfigured build turns into a crash somewhere else entirely.
        """
        source = numpy.arange(4, dtype=numpy.float32)
        descriptor = vtkMemoryDescriptor()
        descriptor.Set(source.ctypes.data, source.nbytes, "level_zero", "data")

        factory = vtkmDataArrayFactory()
        factory.SetNumberOfTuples(4)
        factory.SetNumberOfComponents(1)
        factory.SetDataType(VTK_FLOAT)
        factory.AddBuffer(descriptor)

        with refusal_is_expected():
            self.assertIsNone(factory.CreateArray())

    def testFailureLeavesTheHoldAlone(self):
        """A refused array must not have taken anything with it."""
        source = numpy.arange(4, dtype=numpy.float32)
        descriptor = vtkMemoryDescriptor()
        descriptor.Set(source.ctypes.data, source.nbytes, "level_zero", "data")
        descriptor.SetReleaseAddress(RELEASE_ADDRESS, 11)

        factory = vtkmDataArrayFactory()
        factory.SetNumberOfTuples(4)
        factory.SetNumberOfComponents(1)
        factory.SetDataType(VTK_FLOAT)
        factory.AddBuffer(descriptor)

        with refusal_is_expected():
            self.assertIsNone(factory.CreateArray())
        self.assertTrue(descriptor.HasOwnership())
        self.assertEqual(released, [])

    def testTooManySoAComponentsIsRefused(self):
        """A refusal has to come before any hold is taken.

        Viskores' SoA handles stop at four components. Discovering that
        after the buffers were wrapped would release the producer's memory
        on the way out of a call that reports failure -- and the caller,
        told only that nothing was created, would still believe it owns it.
        """
        components = [numpy.arange(4, dtype=numpy.float32) for _ in range(5)]
        factory = vtkmDataArrayFactory()
        factory.SetNumberOfTuples(4)
        factory.SetNumberOfComponents(5)
        factory.SetDataType(VTK_FLOAT)
        descriptors = [make_descriptor(c, tag=100 + i) for i, c in enumerate(components)]
        for descriptor in descriptors:
            factory.AddBuffer(descriptor)

        with refusal_is_expected():
            self.assertIsNone(factory.CreateArray())
        self.assertEqual(released, [])
        self.assertTrue(all(d.HasOwnership() for d in descriptors))

    def testEveryTypeTheFactoryClaims(self):
        """The dispatch enumerates C types by hand, so gaps are silent.

        `long` and `unsigned long long` were missing, which on LP64 meant
        uint64 could not be wrapped at all -- the factory returned nullptr
        for a type VTK uses natively.
        """
        cases = [(numpy.float32, 10), (numpy.float64, 11), (numpy.int8, 15),
                 (numpy.uint8, 3), (numpy.int16, 4), (numpy.uint16, 5),
                 (numpy.int32, 6), (numpy.uint32, 7), (numpy.int64, 16),
                 (numpy.uint64, 17)]
        for dtype, vtk_type in cases:
            source = numpy.arange(4, dtype=dtype)
            descriptor = make_descriptor(source)
            factory = vtkmDataArrayFactory()
            factory.SetNumberOfTuples(4)
            factory.SetNumberOfComponents(1)
            factory.SetDataType(vtk_type)
            factory.AddBuffer(descriptor)
            self.assertIsNotNone(
                factory.CreateArray(), f"{dtype.__name__} was refused")

    def testAoSDescribesItsOneBuffer(self):
        source = numpy.arange(8, dtype=numpy.float32)
        array = make_array(source, n_components=2)

        descriptors = array.NewMemoryDescriptors()
        self.assertEqual(descriptors.GetNumberOfItems(), 1)
        descriptor = descriptors.GetItemAsObject(0)
        self.assertEqual(descriptor.GetRole(), "data")
        self.assertEqual(descriptor.GetPointer(), source.ctypes.data)

    def testSoADescribesOneBufferPerComponent(self):
        """A struct-of-arrays has a buffer per component and no single
        pointer over the lot. Asked for one, Viskores builds a flattened
        copy -- so the description has to come from the handle instead."""
        components = [numpy.array([0.0, 1.0, 2.0, 3.0], dtype=numpy.float32),
                      numpy.array([10.0, 11.0, 12.0, 13.0], dtype=numpy.float32)]
        factory = vtkmDataArrayFactory()
        factory.SetNumberOfTuples(4)
        factory.SetNumberOfComponents(2)
        factory.SetDataType(VTK_FLOAT)
        for component in components:
            factory.AddBuffer(make_descriptor(component))
        array = factory.CreateArray()

        descriptors = array.NewMemoryDescriptors()
        self.assertEqual(descriptors.GetNumberOfItems(), 2)
        for index, component in enumerate(components):
            descriptor = descriptors.GetItemAsObject(index)
            self.assertEqual(descriptor.GetRole(), f"component_{index}")
            self.assertEqual(descriptor.GetPointer(), component.ctypes.data,
                             f"component {index} described a copy")

        # Aliased, not a snapshot.
        components[0][0] = 99.0
        self.assertEqual(array.GetComponent(0, 0), 99.0)

    def testTheDescriptionDoesNotChangeWhenSomeoneLooks(self):
        """Reading a value swaps the array's handle for a recombined view,
        and that is what it reports from then on. Before this was handled,
        a SoA array described two buffers until read and none after."""
        components = [numpy.arange(4, dtype=numpy.float32),
                      numpy.arange(4, dtype=numpy.float32)]
        factory = vtkmDataArrayFactory()
        factory.SetNumberOfTuples(4)
        factory.SetNumberOfComponents(2)
        factory.SetDataType(VTK_FLOAT)
        for component in components:
            factory.AddBuffer(make_descriptor(component))
        array = factory.CreateArray()

        before = array.NewMemoryDescriptors().GetNumberOfItems()
        array.GetComponent(0, 0)
        array.GetTuple(0)
        after = array.NewMemoryDescriptors().GetNumberOfItems()

        self.assertEqual(before, 2)
        self.assertEqual(after, before, "reading changed what the array says it has")

    def testSoAIsRefusedByDLPack(self):
        """Several buffers cannot be one tensor, and flattening them would
        hand back a snapshot dressed as a view."""
        from vtkmodules.util.dlpack_support import vtk_to_dlpack

        components = [numpy.arange(4, dtype=numpy.float32) for _ in range(2)]
        factory = vtkmDataArrayFactory()
        factory.SetNumberOfTuples(4)
        factory.SetNumberOfComponents(2)
        factory.SetDataType(VTK_FLOAT)
        for component in components:
            factory.AddBuffer(make_descriptor(component))
        array = factory.CreateArray()

        with self.assertRaises(ValueError) as caught:
            vtk_to_dlpack(array)
        self.assertIn("SoA", str(caught.exception))

    def testArrayDoesNotLeakOnDescriptorQuery(self):
        """NewMemoryDescriptors must not pin the array it reports on."""
        array = make_array(numpy.arange(4, dtype=numpy.float32))
        before = array.GetReferenceCount()

        for _ in range(10):
            descriptors = array.NewMemoryDescriptors()
            del descriptors
        gc.collect()

        self.assertEqual(array.GetReferenceCount(), before)


if __name__ == "__main__":
    Testing.main([(TestVtkmDataArrayFactory, "test")])
