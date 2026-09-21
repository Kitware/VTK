"""Zero-copy exchange between VTK arrays and DLPack consumers.

Two things are easy to get wrong here and both fail silently, so both are
checked directly rather than inferred:

  * whether the exchange is actually zero-copy, and
  * whether the memory outlives whoever is looking at it.

Getting lifetime wrong in one direction reads freed memory; getting it
wrong in the other leaks every array that was ever exported. Neither
announces itself.

The device path needs a GPU and a Viskores build, so it is not covered
here; these run anywhere.
"""

import gc
import unittest

from vtkmodules.vtkCommonCore import (
    vtkConstantArray,
    vtkDoubleArray,
    vtkFloatArray,
    vtkIntArray,
    vtkMemoryDescriptor,
    vtkSOADataArrayTemplate,
)
from vtkmodules.util import dlpack_support
from vtkmodules.util.dlpack_support import DLPackArray, dlpack_to_vtk, vtk_to_dlpack
from vtkmodules.test import Testing

try:
    import numpy
except ImportError:
    print("This test requires numpy!")
    Testing.skip()

from vtkmodules.util import numpy_support

# Whether numpy's own DLPack import can hand back a writable view. Asking
# for the versioned protocol -- the only one that can say a tensor is
# writable -- arrived in numpy 2.1; before that every import comes back
# read-only, and a read-only array cannot be re-exported over DLPack at
# all. Both are limits of numpy as a consumer, not of what VTK exported,
# so the tests that turn on them ask rather than assume.
NUMPY_IMPORTS_WRITABLE = numpy.from_dlpack(
    numpy.arange(1, dtype=numpy.float32)).flags.writeable


def wrap(source):
    """A VTK array over *source*'s buffer, so its address is known here.

    Asking the array for its own pointer would answer the question with the
    thing under test; starting from memory the test allocated does not.
    """
    return numpy_support.numpy_to_vtk(source, deep=False)


def make_array(values, n_components=1, cls=vtkFloatArray):
    array = cls()
    array.SetNumberOfComponents(n_components)
    array.SetNumberOfTuples(len(values) // n_components)
    for i, v in enumerate(values):
        array.SetValue(i, v)
    return array


class TestDLPack(Testing.vtkTest):
    def testExportValuesAndShape(self):
        """A 3-component array exports as (tuples, components)."""
        array = make_array(list(range(12)), n_components=3)
        view = numpy.from_dlpack(DLPackArray(array))

        self.assertEqual(view.shape, (4, 3))
        self.assertEqual(view.dtype, numpy.float32)
        numpy.testing.assert_array_equal(view.ravel(), numpy.arange(12))

    def testExportIsZeroCopy(self):
        """The consumer must be looking at the producer's own buffer."""
        source = numpy.array([1.0, 2.0, 3.0, 4.0], dtype=numpy.float32)
        view = numpy.from_dlpack(DLPackArray(wrap(source)))

        self.assertEqual(view.__array_interface__["data"][0], source.ctypes.data)

    def testWritesThroughVTKAreVisible(self):
        """The consequence of zero-copy that actually matters."""
        array = make_array([0.0] * 4)
        view = numpy.from_dlpack(DLPackArray(array))
        array.SetValue(2, 42.0)
        self.assertEqual(view.ravel()[2], 42.0)

    def testDoubleAndInt(self):
        for cls, dtype in ((vtkDoubleArray, numpy.float64), (vtkIntArray, numpy.int32)):
            array = make_array([1, 2, 3], cls=cls)
            view = numpy.from_dlpack(DLPackArray(array))
            self.assertEqual(view.dtype, dtype)

    def testArrayOutlivesItsOwnReference(self):
        """A consumer holding the tensor keeps the VTK array alive.

        Without this the descriptor's reference does nothing and the view
        points into a destroyed array -- which usually still reads, and
        returns whatever now occupies that memory.
        """
        array = make_array([1.0, 2.0, 3.0, 4.0])
        view = numpy.from_dlpack(DLPackArray(array))
        del array
        for _ in range(3):
            gc.collect()
        numpy.testing.assert_array_equal(view.ravel(), [1.0, 2.0, 3.0, 4.0])

    def testConsumedExportsAreReleased(self):
        """Exporting must not pin the array forever."""
        gc.collect()
        before = len(dlpack_support._pinned)
        for _ in range(25):
            array = make_array([1.0, 2.0, 3.0, 4.0])
            view = numpy.from_dlpack(DLPackArray(array))
            del array, view
        gc.collect()
        self.assertEqual(len(dlpack_support._pinned), before)

    def testUnconsumedCapsulesAreReleased(self):
        """A capsule nobody adopts is still somebody's responsibility."""
        gc.collect()
        before = len(dlpack_support._pinned)
        for _ in range(10):
            array = make_array([1.0, 2.0])
            capsule = vtk_to_dlpack(array)
            del capsule, array
        gc.collect()
        self.assertEqual(len(dlpack_support._pinned), before)

    def testImportFromNumpy(self):
        source = numpy.arange(12, dtype=numpy.float32).reshape(4, 3)
        array = dlpack_to_vtk(source, name="imported")

        self.assertEqual(array.GetName(), "imported")
        self.assertEqual(array.GetNumberOfTuples(), 4)
        self.assertEqual(array.GetNumberOfComponents(), 3)
        self.assertEqual(array.GetValue(0), 0.0)
        self.assertEqual(array.GetValue(11), 11.0)

    def testImportIsZeroCopy(self):
        source = numpy.zeros(4, dtype=numpy.float32)
        array = dlpack_to_vtk(source)
        source[1] = 7.0
        self.assertEqual(array.GetValue(1), 7.0)

    def testImportedSourceMayBeDropped(self):
        """The imported array holds the tensor, so the source can go."""
        array = dlpack_to_vtk(numpy.arange(4, dtype=numpy.float32))
        for _ in range(3):
            gc.collect()
        self.assertEqual(array.GetValue(3), 3.0)

    @unittest.skipUnless(NUMPY_IMPORTS_WRITABLE,
                         "numpy < 2.1 marks every DLPack import read-only")
    def testRoundTrip(self):
        original = make_array(list(range(6)), n_components=2)
        back = dlpack_to_vtk(numpy.from_dlpack(DLPackArray(original)))

        self.assertEqual(back.GetNumberOfTuples(), 3)
        self.assertEqual(back.GetNumberOfComponents(), 2)
        for i in range(6):
            self.assertEqual(back.GetValue(i), original.GetValue(i))

    def testNonContiguousIsRejected(self):
        """Wrapping a strided view without copying would misread it."""
        source = numpy.arange(12, dtype=numpy.float32).reshape(4, 3)[:, ::2]
        with self.assertRaises(ValueError):
            dlpack_to_vtk(source)

    def testTooManyDimensionsIsRejected(self):
        source = numpy.zeros((2, 3, 4), dtype=numpy.float32)
        with self.assertRaises(ValueError):
            dlpack_to_vtk(source)

    def testDescriptorReportsHostMemory(self):
        source = numpy.array([1.0, 2.0], dtype=numpy.float32)
        descriptors = wrap(source).NewMemoryDescriptors()

        self.assertEqual(descriptors.GetNumberOfItems(), 1)
        descriptor = descriptors.GetItemAsObject(0)
        self.assertEqual(descriptor.GetMemorySpace(), "host")
        self.assertEqual(descriptor.GetRole(), "data")
        self.assertEqual(descriptor.GetPointer(), source.ctypes.data)
        self.assertEqual(descriptor.GetSizeInBytes(), 2 * 4)

    def testDescriptorHoldsTheArray(self):
        """The descriptor is what makes the pointer safe to hand out."""
        array = make_array([1.0, 2.0, 3.0])
        array.SetName("held")
        descriptors = array.NewMemoryDescriptors()
        descriptor = descriptors.GetItemAsObject(0)

        self.assertIsNotNone(descriptor.GetOwner())
        self.assertTrue(descriptor.HasOwnership())

    def testMetalImportsAsHost(self):
        """A Metal tensor is imported as host memory, and never exported.

        Viskores has no Metal device adapter and vtkDataArray::MemorySpace
        has no Metal value, so no VTK array can hold Metal memory -- there is
        nothing to export. Coming the other way, a shared-storage MTLBuffer on
        Apple Silicon is unified memory, so the pointer is host-readable and
        "host" is not an approximation, it is the truth.
        """
        self.assertEqual(dlpack_support._DEVICE_TO_SPACE[dlpack_support.kDLMetal], "host")
        self.assertNotIn("metal", dlpack_support._SPACE_TO_DEVICE)

    def testOnlyVTKsThreeSpacesAreExportable(self):
        """vtkDataArray::MemorySpace has exactly three values."""
        self.assertEqual(set(dlpack_support._SPACE_TO_DEVICE), {"host", "cuda", "hip"})

    def testNullPointerIsRejected(self):
        """A private-storage Metal buffer reports a null pointer."""
        import ctypes
        from vtkmodules.util.dlpack_support import _DLManagedTensor

        managed = _DLManagedTensor()
        managed.dl_tensor.data = ctypes.c_void_p(0)
        managed.dl_tensor.ndim = 1
        shape = (ctypes.c_int64 * 1)(4)
        managed.dl_tensor.shape = shape
        managed.dl_tensor.dtype.code = dlpack_support.kDLFloat
        managed.dl_tensor.dtype.bits = 32
        managed.dl_tensor.dtype.lanes = 1
        managed.dl_tensor.device.device_type = dlpack_support.kDLMetal

        capsule = dlpack_support._PyCapsule_New(
            ctypes.addressof(managed), b"dltensor", None)
        with self.assertRaises(ValueError):
            dlpack_to_vtk(capsule)

    def testDescriptorReleaseIsCalledOnce(self):
        """SetRelease is the hook a non-VTK producer hangs its lifetime on."""
        import ctypes

        calls = []

        @ctypes.CFUNCTYPE(None, ctypes.c_void_p)
        def release(context):
            calls.append(context)

        descriptor = vtkMemoryDescriptor()
        descriptor.Set(0x1000, 64, "host", "data")
        descriptor.SetReleaseAddress(
            ctypes.cast(release, ctypes.c_void_p).value, 7)
        self.assertTrue(descriptor.HasOwnership())

        del descriptor
        gc.collect()
        self.assertEqual(len(calls), 1)
        self.assertEqual(calls[0], 7)

    def testDescriptorsDoNotLeakTheArray(self):
        """Asking an array where its memory lives must not pin it.

        NewMemoryDescriptors() returns a new collection, and the descriptor
        inside holds a reference to the array. If the wrappers treat that
        collection as borrowed they add a reference that is never dropped,
        so the collection, the descriptor and the array all leak -- and
        vtk_to_dlpack calls this on every single export.

        The reference count is the only visible symptom; nothing fails.
        """
        array = make_array([1.0, 2.0])
        before = array.GetReferenceCount()

        for _ in range(10):
            descriptors = array.NewMemoryDescriptors()
            del descriptors
        gc.collect()

        self.assertEqual(array.GetReferenceCount(), before)

    def testExportDoesNotLeakTheArray(self):
        """The same leak, reached the way callers actually reach it."""
        array = make_array([1.0, 2.0, 3.0, 4.0])
        before = array.GetReferenceCount()

        for _ in range(10):
            view = numpy.from_dlpack(DLPackArray(array))
            del view
        gc.collect()

        self.assertEqual(array.GetReferenceCount(), before)

    def testDescriptorsDescribeOnlyRealStorage(self):
        """An array must not describe memory it does not own.

        A descriptor is a view: whoever holds it writes through it and
        expects the array to see the write. An array with no storage, or
        storage that no single pointer describes, therefore reports nothing
        rather than a flattened copy of itself -- a descriptor over a copy is
        a snapshot of a temporary wearing the shape of a view, and is silent
        in both directions.
        """
        aos = make_array([1.0, 2.0, 3.0, 4.0], n_components=2)
        descriptors = aos.NewMemoryDescriptors()
        self.assertEqual(descriptors.GetNumberOfItems(), 1)
        self.assertEqual(descriptors.GetItemAsObject(0).GetRole(), "data")

        soa = vtkSOADataArrayTemplate["float32"]()
        soa.SetNumberOfComponents(2)
        soa.SetNumberOfTuples(3)
        descriptors = soa.NewMemoryDescriptors()
        self.assertEqual(descriptors.GetNumberOfItems(), 2)
        self.assertEqual(
            [descriptors.GetItemAsObject(i).GetRole() for i in range(2)],
            ["component_0", "component_1"],
        )
        self.assertNotEqual(
            descriptors.GetItemAsObject(0).GetPointer(),
            descriptors.GetItemAsObject(1).GetPointer(),
        )

        implicit = vtkConstantArray["float32"]()
        implicit.ConstructBackend(3.0)
        implicit.SetNumberOfComponents(1)
        implicit.SetNumberOfTuples(4)
        self.assertEqual(
            implicit.NewMemoryDescriptors().GetNumberOfItems(), 0,
            "an array with no storage described some anyway")

    def testSoaIsRefusedRatherThanFlattened(self):
        """DLPack describes one buffer; a struct-of-arrays is several."""
        soa = vtkSOADataArrayTemplate["float32"]()
        soa.SetNumberOfComponents(2)
        soa.SetNumberOfTuples(3)
        for t in range(3):
            for c in range(2):
                soa.SetTypedComponent(t, c, float(t * 2 + c))

        with self.assertRaises(ValueError) as caught:
            vtk_to_dlpack(soa)
        self.assertIn("SoA", str(caught.exception))

    def testImplicitArrayIsRefused(self):
        implicit = vtkConstantArray["float32"]()
        implicit.ConstructBackend(3.0)
        implicit.SetNumberOfComponents(1)
        implicit.SetNumberOfTuples(4)

        with self.assertRaises(ValueError):
            vtk_to_dlpack(implicit)

    def testEveryMappedTypeRoundTrips(self):
        """numpy_to_vtk picks VTK's natively-sized type, which on LP64 means
        vtkLongArray for int64 -- so the map has to carry `long` too, or the
        round trip breaks on exactly one class of platform."""
        for dtype in (numpy.float32, numpy.float64, numpy.int8, numpy.uint8,
                      numpy.int16, numpy.uint16, numpy.int32, numpy.uint32,
                      numpy.int64, numpy.uint64):
            source = numpy.arange(6, dtype=dtype).reshape(3, 2)
            array = dlpack_to_vtk(source)
            back = numpy.from_dlpack(DLPackArray(array))
            numpy.testing.assert_array_equal(
                back.ravel(), numpy.arange(6, dtype=dtype),
                err_msg=f"{dtype.__name__} did not survive")

    def testHostImportIsAoS(self):
        """Host memory goes through numpy_support, so it lands in the layout
        that can actually be described back out again."""
        source = numpy.arange(6, dtype=numpy.float32).reshape(3, 2)
        array = dlpack_to_vtk(source)
        self.assertTrue(array.HasStandardMemoryLayout())
        self.assertEqual(array.NewMemoryDescriptors().GetNumberOfItems(), 1)

    def testSetReleaseIsNotWrapped(self):
        """The reason SetReleaseAddress exists.

        SetRelease takes a function pointer, which the wrapping tools
        cannot express, so it is absent from Python entirely. Nothing warns
        -- it is simply not there, and every import silently failed until
        SetReleaseAddress was added.
        """
        descriptor = vtkMemoryDescriptor()
        self.assertFalse(hasattr(descriptor, "SetRelease"))
        self.assertTrue(hasattr(descriptor, "SetReleaseAddress"))

    @unittest.skipUnless(NUMPY_IMPORTS_WRITABLE,
                         "numpy < 2.1 marks every DLPack import read-only")
    def testExportedViewIsWritable(self):
        """A consumer asking for v1.0 may write back through the view.

        The legacy capsule cannot say whether writing is safe, so numpy
        assumes the worst and marks the view read-only.
        """
        array = make_array([1.0, 2.0, 3.0, 4.0])
        view = numpy.from_dlpack(DLPackArray(array))
        self.assertTrue(view.flags.writeable)
        view[0, 0] = 42.0
        self.assertEqual(array.GetValue(0), 42.0)

    @unittest.skipUnless(NUMPY_IMPORTS_WRITABLE,
                         "numpy < 2.1 marks every DLPack import read-only")
    def testExportedViewCanBeExportedAgain(self):
        """numpy will not re-export a read-only array over DLPack, so
        without the versioned form no round trip through it is possible."""
        array = make_array(list(range(6)), n_components=2)
        again = numpy.from_dlpack(numpy.from_dlpack(DLPackArray(array)))
        numpy.testing.assert_array_equal(again.ravel(), numpy.arange(6))

    def testVersionedCapsuleIsNamedAndStamped(self):
        import ctypes

        array = make_array([1.0, 2.0])
        capsule = DLPackArray(array).__dlpack__(max_version=(1, 0))
        self.assertTrue(dlpack_support._PyCapsule_IsValid(
            ctypes.c_void_p(id(capsule)), b"dltensor_versioned"))

        ptr = dlpack_support._PyCapsule_GetPointer(
            ctypes.c_void_p(id(capsule)), b"dltensor_versioned")
        managed = ctypes.cast(
            ptr, ctypes.POINTER(dlpack_support._DLManagedTensorVersioned)).contents
        self.assertEqual((managed.version.major, managed.version.minor), (1, 0))
        self.assertEqual(managed.flags, 0)

    def testLegacyCapsuleStillExported(self):
        import ctypes

        array = make_array([1.0, 2.0])
        capsule = DLPackArray(array).__dlpack__()
        self.assertTrue(dlpack_support._PyCapsule_IsValid(
            ctypes.c_void_p(id(capsule)), b"dltensor"))

    def testUnconsumedVersionedCapsulesAreReleased(self):
        """The destructor has to recognise both capsule names."""
        gc.collect()
        before = len(dlpack_support._pinned)
        for _ in range(10):
            array = make_array([1.0, 2.0])
            capsule = DLPackArray(array).__dlpack__(max_version=(1, 0))
            del capsule, array
        gc.collect()
        self.assertEqual(len(dlpack_support._pinned), before)

    def testReadOnlyTensorIsRefused(self):
        """A vtkDataArray has no read-only state, so wrapping one would
        hand out a writable array over memory that must not be written."""
        source = numpy.arange(4, dtype=numpy.float32)
        source.flags.writeable = False
        with self.assertRaises(ValueError):
            dlpack_to_vtk(source)

    def testImportedArrayReleasesTheTensor(self):
        """The release has to actually reach the producer."""
        gc.collect()
        before = len(dlpack_support._adopted)
        for _ in range(10):
            array = dlpack_to_vtk(numpy.arange(4, dtype=numpy.float32))
            del array
        gc.collect()
        self.assertEqual(len(dlpack_support._adopted), before)


if __name__ == "__main__":
    Testing.main([(TestDLPack, "test")])
