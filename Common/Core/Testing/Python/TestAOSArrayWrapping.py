"""Test that all vtkAOSDataArrayTemplate subclasses are correctly wrapped
with vtkAOSDataArrayTemplate as their Python superclass, and that basic
typed access (SetValue/GetValue, SetTypedTuple/GetTypedTuple) works."""

from vtkmodules.vtkCommonCore import (
    vtkAOSDataArrayTemplate,
    vtkCharArray,
    vtkDoubleArray,
    vtkFloatArray,
    vtkIdTypeArray,
    vtkIntArray,
    vtkLongArray,
    vtkLongLongArray,
    vtkShortArray,
    vtkSignedCharArray,
    vtkUnsignedCharArray,
    vtkUnsignedIntArray,
    vtkUnsignedLongArray,
    vtkUnsignedLongLongArray,
    vtkUnsignedShortArray,
)
from vtkmodules.test import Testing


# (array class, test value, number of components for tuple test)
# vtkCharArray uses single-character strings; all others use numeric values.
ARRAY_CLASSES = [
    (vtkFloatArray, 3.5, 3),
    (vtkDoubleArray, 3.5, 3),
    (vtkIntArray, 42, 3),
    (vtkUnsignedCharArray, 200, 2),
    (vtkLongLongArray, 123456789, 3),
    (vtkUnsignedIntArray, 100000, 3),
    (vtkShortArray, 1234, 2),
    (vtkUnsignedLongArray, 100000, 3),
    (vtkIdTypeArray, 99999, 3),
    (vtkLongArray, -12345, 3),
    (vtkSignedCharArray, -100, 2),
    (vtkUnsignedLongLongArray, 123456789, 3),
    (vtkUnsignedShortArray, 50000, 2),
]


class TestAOSArrayWrapping(Testing.vtkTest):

    def test_superclass(self):
        """Each array class should have a vtkAOSDataArrayTemplate
        instantiation as its direct Python superclass."""
        all_classes = ARRAY_CLASSES + [(vtkCharArray, None, None)]
        for cls, _, _ in all_classes:
            base = cls.__bases__[0]
            self.assertIn(
                "vtkAOSDataArrayTemplate",
                base.__name__,
                f"{cls.__name__} base class is {base.__name__}, "
                f"expected vtkAOSDataArrayTemplate",
            )

    def test_set_get_value(self):
        """SetValue/GetValue should round-trip correctly for numeric types."""
        for cls, val, _ in ARRAY_CLASSES:
            arr = cls()
            arr.SetNumberOfTuples(5)
            arr.SetValue(2, val)
            result = arr.GetValue(2)
            self.assertEqual(
                result,
                val,
                f"{cls.__name__}: SetValue/GetValue failed, "
                f"expected {val}, got {result}",
            )

    def test_set_get_value_char(self):
        """SetValue/GetValue should round-trip for vtkCharArray (character type)."""
        arr = vtkCharArray()
        arr.SetNumberOfTuples(5)
        arr.SetValue(2, "A")
        self.assertEqual(arr.GetValue(2), "A")

    def test_set_get_typed_tuple(self):
        """SetTypedTuple/GetTypedTuple should round-trip correctly."""
        for cls, val, ncomp in ARRAY_CLASSES:
            arr = cls()
            arr.SetNumberOfComponents(ncomp)
            arr.SetNumberOfTuples(3)
            tup_in = tuple(val + i for i in range(ncomp))
            arr.SetTypedTuple(1, tup_in)
            tup_out = [type(val)(0)] * ncomp
            arr.GetTypedTuple(1, tup_out)
            self.assertEqual(
                tuple(tup_out),
                tup_in,
                f"{cls.__name__}: SetTypedTuple/GetTypedTuple failed, "
                f"expected {tup_in}, got {tuple(tup_out)}",
            )

    def test_get_buffer(self):
        """GetBuffer should return a vtkBuffer object."""
        all_classes = ARRAY_CLASSES + [(vtkCharArray, None, None)]
        for cls, _, _ in all_classes:
            arr = cls()
            arr.SetNumberOfTuples(1)
            buf = arr.GetBuffer()
            self.assertIn(
                "vtkBuffer",
                type(buf).__name__,
                f"{cls.__name__}: GetBuffer returned {type(buf).__name__}, "
                f"expected vtkBuffer",
            )

    def test_insert_next_value(self):
        """InsertNextValue should append and be retrievable."""
        for cls, val, _ in ARRAY_CLASSES:
            arr = cls()
            idx = arr.InsertNextValue(val)
            self.assertEqual(idx, 0)
            self.assertEqual(
                arr.GetValue(0),
                val,
                f"{cls.__name__}: InsertNextValue/GetValue failed",
            )


if __name__ == "__main__":
    Testing.main([(TestAOSArrayWrapping, "test")])
