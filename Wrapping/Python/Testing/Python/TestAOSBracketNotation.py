"""Test bracket notation for vtkAOSDataArrayTemplate.

Verifies that vtkAOSDataArrayTemplate can be instantiated using
bracket notation (e.g., vtkAOSDataArrayTemplate['float32']()) and
that the resulting arrays work correctly for all supported types.
"""

from vtkmodules.vtkCommonCore import vtkAOSDataArrayTemplate
from vtkmodules.test import Testing


types = ['char', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32',
         'int64', 'uint64', 'float32', 'float64']


class TestAOSBracketNotation(Testing.vtkTest):
    """Test vtkAOSDataArrayTemplate bracket notation."""

    def testInstantiation(self):
        """All supported types can be instantiated via bracket notation."""
        for T in types:
            arr = vtkAOSDataArrayTemplate[T]()
            self.assertIsNotNone(arr)

    def testSetGetTuple(self):
        """Set/Get tuple works for each type."""
        for T in types:
            arr = vtkAOSDataArrayTemplate[T]()
            arr.SetNumberOfComponents(3)
            arr.SetNumberOfTuples(2)
            arr.SetTuple(0, (1, 2, 3))
            self.assertEqual(arr.GetTuple(0), (1.0, 2.0, 3.0))

    def testTypedInterface(self):
        """Typed component access works for non-char types."""
        for T in types:
            if T == 'char':
                continue
            arr = vtkAOSDataArrayTemplate[T]()
            arr.SetNumberOfComponents(3)
            arr.SetNumberOfTuples(1)
            arr.SetTypedTuple(0, (3, 4, 6))
            t = [0, 0, 0]
            arr.GetTypedTuple(0, t)
            self.assertEqual(t, [3, 4, 6])
            self.assertEqual(arr.GetTypedComponent(0, 0), 3)
            self.assertEqual(arr.GetTypedComponent(0, 1), 4)
            self.assertEqual(arr.GetTypedComponent(0, 2), 6)

    def testKeys(self):
        """keys() returns the list of available type strings."""
        keys = vtkAOSDataArrayTemplate.keys()
        for T in types:
            self.assertIn(T, keys)

    def testGetBuffer(self):
        """GetBuffer returns a valid buffer for bracket-instantiated arrays."""
        arr = vtkAOSDataArrayTemplate['float64']()
        arr.SetNumberOfComponents(2)
        arr.SetNumberOfTuples(3)
        for i in range(6):
            arr.SetValue(i, float(i))

        buf = arr.GetBuffer()
        self.assertIsNotNone(buf)
        self.assertEqual(buf.GetNumberOfElements(), 6)

    def testMemoryView(self):
        """memoryview works on bracket-instantiated arrays."""
        arr = vtkAOSDataArrayTemplate['float32']()
        arr.SetNumberOfComponents(1)
        arr.SetNumberOfTuples(3)
        arr.SetValue(0, 10.0)
        arr.SetValue(1, 20.0)
        arr.SetValue(2, 30.0)

        m = memoryview(arr)
        self.assertEqual(m.shape, (3,))


if __name__ == "__main__":
    Testing.main([(TestAOSBracketNotation, 'test')])
