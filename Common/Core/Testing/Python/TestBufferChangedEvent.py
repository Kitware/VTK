"""Test BufferChangedEvent for data array templates.

BufferChangedEvent is fired by ReallocateTuples when the internal buffer
of a data array is reallocated. This test verifies the event fires correctly
for vtkAOSDataArrayTemplate, vtkSOADataArrayTemplate, and
vtkScaledSOADataArrayTemplate.
"""

from vtkmodules.vtkCommonCore import (
    vtkCommand,
    vtkFloatArray,
    vtkSOADataArrayTemplate,
    vtkScaledSOADataArrayTemplate,
)
from vtkmodules.test import Testing


class TestBufferChangedEvent(Testing.vtkTest):
    """Test that BufferChangedEvent fires on buffer reallocation."""

    def _make_observer(self):
        """Create an observer callback that counts invocations."""
        state = {"count": 0}
        def callback(obj, event):
            state["count"] += 1
        return callback, state

    # ------------------------------------------------------------------
    # AOS array (vtkFloatArray wraps vtkAOSDataArrayTemplate<float>)
    # ------------------------------------------------------------------

    def testAOSBufferChangedOnResize(self):
        """AOS: BufferChangedEvent fires when Resize causes reallocation."""
        arr = vtkFloatArray()
        arr.SetNumberOfComponents(1)
        arr.SetNumberOfTuples(4)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        # Resize to a significantly larger size to trigger reallocation
        arr.Resize(1000)
        self.assertGreater(state["count"], 0,
            "BufferChangedEvent should fire when AOS array is resized")

    def testAOSBufferChangedOnInsert(self):
        """AOS: BufferChangedEvent fires when InsertNextTuple reallocates."""
        arr = vtkFloatArray()
        arr.SetNumberOfComponents(1)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        # Insert enough values to force at least one reallocation
        for i in range(1000):
            arr.InsertNextTuple1(float(i))

        self.assertGreater(state["count"], 0,
            "BufferChangedEvent should fire during InsertNextTuple growth")

    def testAOSNoEventOnSameSize(self):
        """AOS: BufferChangedEvent does not fire when size is unchanged."""
        arr = vtkFloatArray()
        arr.SetNumberOfComponents(1)
        arr.SetNumberOfTuples(10)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        # Set to the same number of tuples - should not reallocate
        arr.SetNumberOfTuples(10)
        self.assertEqual(state["count"], 0,
            "BufferChangedEvent should not fire when size is unchanged")

    # ------------------------------------------------------------------
    # SOA array (vtkSOADataArrayTemplate<float>)
    # ------------------------------------------------------------------

    def testSOABufferChangedOnResize(self):
        """SOA: BufferChangedEvent fires when Resize causes reallocation."""
        arr = vtkSOADataArrayTemplate['float32']()
        arr.SetNumberOfComponents(3)
        arr.SetNumberOfTuples(4)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        arr.Resize(1000)
        self.assertGreater(state["count"], 0,
            "BufferChangedEvent should fire when SOA array is resized")

    def testSOABufferChangedOnInsert(self):
        """SOA: BufferChangedEvent fires when InsertNextTuple reallocates."""
        arr = vtkSOADataArrayTemplate['float32']()
        arr.SetNumberOfComponents(3)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        for i in range(1000):
            arr.InsertNextTuple3(float(i), float(i), float(i))

        self.assertGreater(state["count"], 0,
            "BufferChangedEvent should fire during InsertNextTuple growth")

    def testSOANoEventOnSameSize(self):
        """SOA: BufferChangedEvent does not fire when size is unchanged."""
        arr = vtkSOADataArrayTemplate['float32']()
        arr.SetNumberOfComponents(3)
        arr.SetNumberOfTuples(10)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        arr.SetNumberOfTuples(10)
        self.assertEqual(state["count"], 0,
            "BufferChangedEvent should not fire when size is unchanged")

    # ------------------------------------------------------------------
    # ScaledSOA array (vtkScaledSOADataArrayTemplate<float>)
    # ------------------------------------------------------------------

    def testScaledSOABufferChangedOnResize(self):
        """ScaledSOA: BufferChangedEvent fires when Resize reallocates."""
        arr = vtkScaledSOADataArrayTemplate['float32']()
        arr.SetNumberOfComponents(3)
        arr.SetNumberOfTuples(4)
        arr.SetScale(2.0)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        arr.Resize(1000)
        self.assertGreater(state["count"], 0,
            "BufferChangedEvent should fire when ScaledSOA array is resized")

    def testScaledSOABufferChangedOnInsert(self):
        """ScaledSOA: BufferChangedEvent fires when InsertNextTuple reallocates."""
        arr = vtkScaledSOADataArrayTemplate['float32']()
        arr.SetNumberOfComponents(3)
        arr.SetScale(2.0)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        for i in range(1000):
            arr.InsertNextTuple3(float(i), float(i), float(i))

        self.assertGreater(state["count"], 0,
            "BufferChangedEvent should fire during InsertNextTuple growth")

    def testScaledSOANoEventOnSameSize(self):
        """ScaledSOA: BufferChangedEvent does not fire when size unchanged."""
        arr = vtkScaledSOADataArrayTemplate['float32']()
        arr.SetNumberOfComponents(3)
        arr.SetNumberOfTuples(10)
        arr.SetScale(2.0)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        arr.SetNumberOfTuples(10)
        self.assertEqual(state["count"], 0,
            "BufferChangedEvent should not fire when size is unchanged")

    # ------------------------------------------------------------------
    # Observer removal
    # ------------------------------------------------------------------

    def testObserverRemoval(self):
        """Event stops firing after observer is removed."""
        arr = vtkFloatArray()
        arr.SetNumberOfComponents(1)
        arr.SetNumberOfTuples(4)

        cb, state = self._make_observer()
        obs_id = arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        arr.Resize(1000)
        count_after_first = state["count"]
        self.assertGreater(count_after_first, 0)

        arr.RemoveObserver(obs_id)
        arr.Resize(10000)
        self.assertEqual(state["count"], count_after_first,
            "BufferChangedEvent should not fire after observer is removed")


if __name__ == "__main__":
    Testing.main([(TestBufferChangedEvent, 'test')])
