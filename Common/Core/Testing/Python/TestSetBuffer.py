"""Test SetBuffer and BufferChangedEvent for AOS and SOA data array templates.

Verifies that SetBuffer() correctly transfers buffer data, and that
BufferChangedEvent fires from SetBuffer, SetArray, and ShallowCopy.
"""

from vtkmodules.vtkCommonCore import (
    vtkCommand,
    vtkDoubleArray,
    vtkFloatArray,
    vtkSOADataArrayTemplate,
)
from vtkmodules.test import Testing


class TestSetBuffer(Testing.vtkTest):
    """Test SetBuffer and BufferChangedEvent for AOS and SOA arrays."""

    def _make_observer(self):
        """Create an observer callback that counts invocations."""
        state = {"count": 0}

        def callback(obj, event):
            state["count"] += 1

        return callback, state

    # ------------------------------------------------------------------
    # AOS SetBuffer
    # ------------------------------------------------------------------

    def testAOSSetBuffer(self):
        """AOS: SetBuffer transfers data correctly."""
        src = vtkFloatArray()
        src.SetNumberOfComponents(2)
        src.SetNumberOfTuples(3)
        for i in range(6):
            src.SetValue(i, float(i + 1))

        buf = src.GetBuffer()

        dst = vtkFloatArray()
        dst.SetNumberOfComponents(2)
        dst.SetBuffer(buf, True)

        self.assertEqual(dst.GetNumberOfTuples(), 3)
        for i in range(6):
            self.assertAlmostEqual(dst.GetValue(i), float(i + 1))

        t = dst.GetTuple(1)
        self.assertAlmostEqual(t[0], 3.0)
        self.assertAlmostEqual(t[1], 4.0)

    def testAOSSetBufferEvent(self):
        """AOS: SetBuffer fires BufferChangedEvent."""
        src = vtkFloatArray()
        src.SetNumberOfComponents(1)
        src.SetNumberOfTuples(4)

        dst = vtkFloatArray()
        dst.SetNumberOfComponents(1)

        cb, state = self._make_observer()
        dst.AddObserver(vtkCommand.BufferChangedEvent, cb)

        dst.SetBuffer(src.GetBuffer(), True)
        self.assertGreater(state["count"], 0,
                           "BufferChangedEvent should fire on AOS SetBuffer")

    def testAOSSetArrayEvent(self):
        """AOS: SetArray fires BufferChangedEvent."""
        arr = vtkFloatArray()
        arr.SetNumberOfComponents(1)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        arr.SetNumberOfTuples(4)
        state["count"] = 0  # reset after allocation events

        other = vtkFloatArray()
        other.SetNumberOfComponents(1)
        other.SetNumberOfTuples(4)
        arr.SetArray(other, 4, True)

        self.assertGreater(state["count"], 0,
                           "BufferChangedEvent should fire on AOS SetArray")

    def testAOSShallowCopyEvent(self):
        """AOS: ShallowCopy fires BufferChangedEvent."""
        src = vtkFloatArray()
        src.SetNumberOfComponents(2)
        src.SetNumberOfTuples(3)

        dst = vtkFloatArray()
        dst.SetNumberOfComponents(2)

        cb, state = self._make_observer()
        dst.AddObserver(vtkCommand.BufferChangedEvent, cb)

        dst.ShallowCopy(src)
        self.assertGreater(state["count"], 0,
                           "BufferChangedEvent should fire on AOS ShallowCopy")

    # ------------------------------------------------------------------
    # SOA SetBuffer
    # ------------------------------------------------------------------

    def testSOASetBuffer(self):
        """SOA: SetBuffer transfers per-component data correctly."""
        numTuples = 4
        # Create source arrays to get buffers from
        comp0 = vtkDoubleArray()
        comp0.SetNumberOfComponents(1)
        comp0.SetNumberOfTuples(numTuples)
        for i in range(numTuples):
            comp0.SetValue(i, float(i))

        comp1 = vtkDoubleArray()
        comp1.SetNumberOfComponents(1)
        comp1.SetNumberOfTuples(numTuples)
        for i in range(numTuples):
            comp1.SetValue(i, float(10 + i))

        arr = vtkSOADataArrayTemplate['float64']()
        arr.SetNumberOfComponents(2)
        arr.SetBuffer(0, comp0.GetBuffer(), True)
        arr.SetBuffer(1, comp1.GetBuffer(), False)

        self.assertEqual(arr.GetNumberOfTuples(), numTuples)
        t = arr.GetTuple(2)
        self.assertAlmostEqual(t[0], 2.0)
        self.assertAlmostEqual(t[1], 12.0)

    def testSOASetBufferEvent(self):
        """SOA: SetBuffer fires BufferChangedEvent."""
        arr = vtkSOADataArrayTemplate['float64']()
        arr.SetNumberOfComponents(2)
        arr.SetNumberOfTuples(1)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        comp = vtkDoubleArray()
        comp.SetNumberOfComponents(1)
        comp.SetNumberOfTuples(4)
        arr.SetBuffer(0, comp.GetBuffer(), True)

        self.assertGreater(state["count"], 0,
                           "BufferChangedEvent should fire on SOA SetBuffer")

    def testSOASetArrayEvent(self):
        """SOA: SetArray fires BufferChangedEvent."""
        arr = vtkSOADataArrayTemplate['float64']()
        arr.SetNumberOfComponents(2)
        arr.SetNumberOfTuples(1)

        cb, state = self._make_observer()
        arr.AddObserver(vtkCommand.BufferChangedEvent, cb)

        other = vtkDoubleArray()
        other.SetNumberOfComponents(1)
        other.SetNumberOfTuples(4)
        arr.SetArray(0, other, 4, False, True)

        self.assertGreater(state["count"], 0,
                           "BufferChangedEvent should fire on SOA SetArray")

    def testSOAShallowCopyEvent(self):
        """SOA: ShallowCopy fires BufferChangedEvent."""
        src = vtkSOADataArrayTemplate['float64']()
        src.SetNumberOfComponents(2)
        src.SetNumberOfTuples(3)

        dst = vtkSOADataArrayTemplate['float64']()
        dst.SetNumberOfComponents(2)

        cb, state = self._make_observer()
        dst.AddObserver(vtkCommand.BufferChangedEvent, cb)

        dst.ShallowCopy(src)
        self.assertGreater(state["count"], 0,
                           "BufferChangedEvent should fire on SOA ShallowCopy")


if __name__ == "__main__":
    Testing.main([(TestSetBuffer, 'test')])
