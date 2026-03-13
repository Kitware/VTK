"""Test memory safety features for SOA data arrays.

This test verifies that:
1. BufferChangedEvent fires when SOA buffers are replaced or cleared
2. VTKSOAArray detects buffer changes via observer
3. Buffer references keep numpy memory valid

Created on Feb 5, 2026
"""

from vtkmodules.vtkCommonCore import (
    vtkCommand,
    vtkSOADataArrayTemplate,
)
from vtkmodules.test import Testing

try:
    import numpy
except ImportError:
    print("This test requires numpy!")
    Testing.skip()


class TestSOAMemorySafety(Testing.vtkTest):
    """Test SOA data array memory safety features."""

    def testBufferChangedEventOnSetArray(self):
        """Test that BufferChangedEvent fires when SetArray() replaces a buffer."""
        arr = vtkSOADataArrayTemplate[numpy.float64]()
        arr.SetNumberOfComponents(2)

        # Set up initial SOA data
        comp0 = numpy.arange(10, dtype=numpy.float64)
        comp1 = numpy.arange(10, 20, dtype=numpy.float64)
        arr.SetArray(0, comp0, 10, True, True)
        arr.SetArray(1, comp1, 10, False, True)

        # Now add observer after initial setup
        event_fired = [False]
        def on_buffer_changed(obj, event):
            event_fired[0] = True

        arr.AddObserver(vtkCommand.BufferChangedEvent, on_buffer_changed)

        # SetArray should fire BufferChangedEvent
        new_data = numpy.arange(100, 110, dtype=numpy.float64)
        arr.SetArray(0, new_data, 10, True, True)

        self.assertTrue(event_fired[0],
            "BufferChangedEvent should fire when SetArray() replaces a buffer")

    def testBufferChangedEventOnReallocateTuples(self):
        """Test that BufferChangedEvent fires when ReallocateTuples() changes buffer."""
        arr = vtkSOADataArrayTemplate[numpy.float64]()
        arr.SetNumberOfComponents(2)

        # Set up initial SOA data
        comp0 = numpy.arange(10, dtype=numpy.float64)
        comp1 = numpy.arange(10, 20, dtype=numpy.float64)
        arr.SetArray(0, comp0, 10, True, True)
        arr.SetArray(1, comp1, 10, False, True)

        event_fired = [False]
        def on_buffer_changed(obj, event):
            event_fired[0] = True

        arr.AddObserver(vtkCommand.BufferChangedEvent, on_buffer_changed)

        # ReserveTuples should fire BufferChangedEvent (triggers ReallocateTuples)
        arr.ReserveTuples(100)

        self.assertTrue(event_fired[0],
            "BufferChangedEvent should fire when ReallocateTuples() changes buffer")

    def testVTKSOAArrayObserverDetectsBufferChange(self):
        """Test that VTKSOAArray re-initializes after buffer changes."""
        arr = vtkSOADataArrayTemplate[numpy.float64]()
        arr.SetNumberOfComponents(2)

        # Set up SOA data
        comp0 = numpy.arange(10, dtype=numpy.float64)
        comp1 = numpy.arange(10, 20, dtype=numpy.float64)
        arr.SetArray(0, comp0, 10, True, True)
        arr.SetArray(1, comp1, 10, False, True)

        # With mixin, arr IS already a VTKSOAArray
        from vtkmodules.numpy_interface.vtk_soa_array import VTKSOAArray
        self.assertIsInstance(arr, VTKSOAArray)

        # Trigger lazy init by accessing component arrays
        self.assertEqual(len(arr), 10)
        _ = arr._component_arrays

        # Track if observer was triggered (observer removes itself after firing)
        observer_id = arr._observer_id

        # Replacing a buffer triggers BufferChangedEvent, which invalidates
        # the cache. The array should seamlessly re-initialize on next access.
        new_data = numpy.arange(100, 110, dtype=numpy.float64)
        arr.SetArray(0, new_data, 10, True, True)

        # Observer should have removed itself after firing
        self.assertIsNone(observer_id[0],
            "Observer should remove itself after BufferChangedEvent fires")

        # Array should still work — accessing it re-initializes the cache
        self.assertEqual(arr[0, 0], 100.0,
            "Array should reflect new buffer data after SetArray")
        self.assertEqual(arr[0, 1], 10.0,
            "Unchanged component should still be accessible")
        result = arr + 1
        self.assertEqual(result[0, 0], 101.0)

    def testVTKSOAArrayObserverDetectsResize(self):
        """Test that VTKSOAArray re-initializes after resize."""
        arr = vtkSOADataArrayTemplate[numpy.float64]()
        arr.SetNumberOfComponents(2)

        # Set up SOA data
        comp0 = numpy.arange(10, dtype=numpy.float64)
        comp1 = numpy.arange(10, 20, dtype=numpy.float64)
        arr.SetArray(0, comp0, 10, True, True)
        arr.SetArray(1, comp1, 10, False, True)

        # Trigger lazy init
        self.assertEqual(len(arr), 10)
        _ = arr._component_arrays

        # Track observer ID
        observer_id = arr._observer_id

        # ReserveTuples triggers BufferChangedEvent, invalidating cache
        arr.ReserveTuples(100)

        # Observer should have removed itself after firing
        self.assertIsNone(observer_id[0],
            "Observer should remove itself after BufferChangedEvent fires")

        # Array should still work after resize
        self.assertEqual(len(arr), 10,
            "Array length should be unchanged after Resize (capacity change)")

    def testBufferReferencesKeepMemoryValid(self):
        """Test that buffer references keep numpy memory valid."""
        from vtkmodules.util import numpy_support

        # Create numpy arrays
        comp0 = numpy.arange(10, dtype=numpy.float64)
        comp1 = numpy.arange(10, 20, dtype=numpy.float64)

        # Create SOA array from numpy
        vtk_arr = numpy_support.numpy_to_vtk_soa([comp0, comp1], name="test")

        # Verify references are stored on buffers
        buf0 = vtk_arr.GetComponentBuffer(0)
        buf1 = vtk_arr.GetComponentBuffer(1)

        self.assertTrue(hasattr(buf0, '_numpy_reference'),
            "Buffer should have _numpy_reference attribute")
        self.assertTrue(hasattr(buf1, '_numpy_reference'),
            "Buffer should have _numpy_reference attribute")

        # Verify data is correct
        np_buf0 = numpy.asarray(buf0)
        np_buf1 = numpy.asarray(buf1)
        self.assertTrue(numpy.array_equal(np_buf0, comp0))
        self.assertTrue(numpy.array_equal(np_buf1, comp1))

    def testVTKSOAArrayStoresBufferReferences(self):
        """Test that VTKSOAArray stores buffer references."""
        arr = vtkSOADataArrayTemplate[numpy.float64]()
        arr.SetNumberOfComponents(2)

        # Set up SOA data
        comp0 = numpy.arange(10, dtype=numpy.float64)
        comp1 = numpy.arange(10, 20, dtype=numpy.float64)
        arr.SetArray(0, comp0, 10, True, True)
        arr.SetArray(1, comp1, 10, False, True)

        # Trigger lazy init (sets up observer and buffer references)
        _ = arr._component_arrays

        # Check that buffer references are stored
        self.assertEqual(len(arr._buffers), 2,
            "VTKSOAArray should store references to component buffers")

    def testObserverCleanupOnDelete(self):
        """Test that observer is cleaned up when VTKSOAArray is deleted."""
        import gc

        arr = vtkSOADataArrayTemplate[numpy.float64]()
        arr.SetNumberOfComponents(2)

        # Set up SOA data (must use SetArray for SOA mode)
        comp0 = numpy.arange(10, dtype=numpy.float64)
        comp1 = numpy.arange(10, 20, dtype=numpy.float64)
        arr.SetArray(0, comp0, 10, True, True)
        arr.SetArray(1, comp1, 10, False, True)

        # Trigger lazy init to set up observer
        _ = arr._component_arrays

        # Delete the array reference and force garbage collection
        del arr
        gc.collect()

        # Create a new array to verify no lingering issues
        arr2 = vtkSOADataArrayTemplate[numpy.float64]()
        arr2.SetNumberOfComponents(1)
        arr2.SetNumberOfTuples(5)
        self.assertEqual(arr2.GetNumberOfTuples(), 5)


if __name__ == "__main__":
    Testing.main([(TestSOAMemorySafety, 'test')])
