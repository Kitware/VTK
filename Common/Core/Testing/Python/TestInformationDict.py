"""Test dictionary-style interface for vtkInformation."""

from vtkmodules.vtkCommonCore import vtkInformation, vtkInformationVector
from vtkmodules.vtkCommonDataModel import vtkDataObject, vtkPolyData
from vtkmodules.vtkCommonExecutionModel import (
    vtkStreamingDemandDrivenPipeline as SDD,
    vtkAlgorithm,
)
from vtkmodules.test import Testing

# Use built-in VTK keys to avoid shutdown crashes from dynamic MakeKey.
# Scalar keys
INT_KEY = vtkDataObject.FIELD_ASSOCIATION()          # IntegerKey
DOUBLE_KEY = vtkDataObject.DATA_TIME_STEP()          # DoubleKey
STRING_KEY = vtkDataObject.FIELD_NAME()              # StringKey

# Vector keys
INT_VEC_KEY = vtkDataObject.ALL_PIECES_EXTENT()      # IntegerVectorKey
DOUBLE_VEC_KEY = vtkDataObject.BOUNDING_BOX()        # DoubleVectorKey

# Request key
REQUEST_KEY = SDD.REQUEST_DATA()                     # RequestKey

# DataObject key
DATA_OBJ_KEY = vtkDataObject.DATA_OBJECT()           # DataObjectKey

# InformationVector key
INFO_VEC_KEY = vtkDataObject.CELL_DATA_VECTOR()      # InformationVectorKey

# KeyVector key
KEY_VEC_KEY = SDD.KEYS_TO_COPY()                     # KeyVectorKey


class TestInformationDict(Testing.vtkTest):
    def setUp(self):
        self.info = vtkInformation()

    # --- Scalar keys ---

    def test_integer_key(self):
        self.info[INT_KEY] = 42
        self.assertEqual(self.info[INT_KEY], 42)

    def test_double_key(self):
        self.info[DOUBLE_KEY] = 3.14
        self.assertAlmostEqual(self.info[DOUBLE_KEY], 3.14)

    def test_string_key(self):
        self.info[STRING_KEY] = "hello"
        self.assertEqual(self.info[STRING_KEY], "hello")

    # --- Vector keys ---

    def test_integer_vector_key(self):
        self.info[INT_VEC_KEY] = (1, 2, 3, 4, 5, 6)
        self.assertEqual(self.info[INT_VEC_KEY], (1, 2, 3, 4, 5, 6))

    def test_double_vector_key(self):
        self.info[DOUBLE_VEC_KEY] = (1.0, 2.0, 3.0, 4.0, 5.0, 6.0)
        result = self.info[DOUBLE_VEC_KEY]
        self.assertEqual(len(result), 6)
        for a, b in zip(result, (1.0, 2.0, 3.0, 4.0, 5.0, 6.0)):
            self.assertAlmostEqual(a, b)

    def test_vector_overwrite(self):
        """Setting a vector key replaces previous values."""
        self.info[INT_VEC_KEY] = (1, 2, 3, 4, 5, 6)
        self.info[INT_VEC_KEY] = (10, 20, 30, 40, 50, 60)
        self.assertEqual(self.info[INT_VEC_KEY], (10, 20, 30, 40, 50, 60))

    # --- Request key ---

    def test_request_key_set_true(self):
        self.info[REQUEST_KEY] = True
        self.assertIn(REQUEST_KEY, self.info)
        self.assertTrue(self.info[REQUEST_KEY])

    def test_request_key_set_false(self):
        self.info[REQUEST_KEY] = True
        self.info[REQUEST_KEY] = False
        self.assertNotIn(REQUEST_KEY, self.info)

    # --- Object keys ---

    def test_data_object_key(self):
        obj = vtkPolyData()
        self.info[DATA_OBJ_KEY] = obj
        result = self.info[DATA_OBJ_KEY]
        self.assertIsInstance(result, vtkPolyData)

    def test_information_vector_key(self):
        vec = vtkInformationVector()
        self.info[INFO_VEC_KEY] = vec
        result = self.info[INFO_VEC_KEY]
        self.assertIsNotNone(result)

    # --- KeyVector key ---

    def test_key_vector_key(self):
        self.info[KEY_VEC_KEY] = (INT_KEY, STRING_KEY)
        result = self.info[KEY_VEC_KEY]
        self.assertEqual(len(result), 2)

    # --- __contains__ ---

    def test_contains_present(self):
        self.info[INT_KEY] = 10
        self.assertIn(INT_KEY, self.info)

    def test_contains_absent(self):
        self.assertNotIn(INT_KEY, self.info)

    # --- __delitem__ ---

    def test_delitem(self):
        self.info[INT_KEY] = 10
        del self.info[INT_KEY]
        self.assertNotIn(INT_KEY, self.info)

    def test_delitem_missing_raises(self):
        with self.assertRaises(KeyError):
            del self.info[INT_KEY]

    # --- __getitem__ missing raises ---

    def test_getitem_missing_raises(self):
        with self.assertRaises(KeyError):
            _ = self.info[INT_KEY]

    # --- __len__ ---

    def test_len_empty(self):
        self.assertEqual(len(self.info), 0)

    def test_len(self):
        self.info[INT_KEY] = 1
        self.info[STRING_KEY] = "x"
        self.assertEqual(len(self.info), 2)

    # --- __iter__ ---

    def test_iter(self):
        self.info[INT_KEY] = 1
        self.info[STRING_KEY] = "x"
        key_list = list(self.info)
        self.assertEqual(len(key_list), 2)

    # --- keys / values / items ---

    def test_keys(self):
        self.info[INT_KEY] = 42
        self.info[STRING_KEY] = "hello"
        ks = self.info.keys()
        self.assertEqual(len(ks), 2)
        self.assertTrue(all(isinstance(k, str) for k in ks))
        self.assertIn("FIELD_ASSOCIATION", ks)
        self.assertIn("FIELD_NAME", ks)

    def test_values(self):
        self.info[INT_KEY] = 42
        vals = self.info.values()
        self.assertIn(42, vals)

    def test_items(self):
        self.info[INT_KEY] = 42
        its = self.info.items()
        self.assertEqual(len(its), 1)
        key, val = its[0]
        self.assertIsInstance(key, str)
        self.assertEqual(val, 42)

    # --- get ---

    def test_get_present(self):
        self.info[INT_KEY] = 42
        self.assertEqual(self.info.get(INT_KEY), 42)

    def test_get_missing_default(self):
        self.assertIsNone(self.info.get(INT_KEY))
        self.assertEqual(self.info.get(INT_KEY, -1), -1)

    # --- update ---

    def test_update(self):
        other = vtkInformation()
        other[INT_KEY] = 99
        other[STRING_KEY] = "copied"
        self.info.update(other)
        self.assertEqual(self.info[INT_KEY], 99)
        self.assertEqual(self.info[STRING_KEY], "copied")

    # --- clear ---

    def test_clear(self):
        self.info[INT_KEY] = 1
        self.info[STRING_KEY] = "x"
        self.info.clear()
        self.assertEqual(len(self.info), 0)

    # --- __repr__ ---

    def test_repr(self):
        self.info[INT_KEY] = 42
        r = repr(self.info)
        self.assertIn("vtkInformation", r)
        self.assertIn("42", r)

    def test_repr_empty(self):
        r = repr(self.info)
        self.assertEqual(r, "vtkInformation({})")

    # --- string key lookup ---

    def test_string_key_getset(self):
        """String key names resolve to the correct vtkInformationKey."""
        self.info["FIELD_NAME"] = "Pressure"
        self.assertEqual(self.info["FIELD_NAME"], "Pressure")
        # Cross-check with actual key object
        self.assertEqual(self.info[STRING_KEY], "Pressure")

    def test_string_key_contains(self):
        self.assertNotIn("FIELD_NAME", self.info)
        self.info["FIELD_NAME"] = "Velocity"
        self.assertIn("FIELD_NAME", self.info)

    def test_string_key_delitem(self):
        self.info["FIELD_NAME"] = "Temp"
        del self.info["FIELD_NAME"]
        self.assertNotIn("FIELD_NAME", self.info)

    def test_string_key_get(self):
        self.assertIsNone(self.info.get("FIELD_NAME"))
        self.info["FIELD_NAME"] = "test"
        self.assertEqual(self.info.get("FIELD_NAME"), "test")

    def test_string_key_integer(self):
        self.info["FIELD_ASSOCIATION"] = 2
        self.assertEqual(self.info["FIELD_ASSOCIATION"], 2)

    def test_string_key_case_insensitive(self):
        self.info["field_name"] = "LowerCase"
        self.assertEqual(self.info["Field_Name"], "LowerCase")
        self.assertEqual(self.info[STRING_KEY], "LowerCase")

    def test_string_key_not_found(self):
        with self.assertRaises(KeyError):
            _ = self.info["NO_SUCH_KEY_EXISTS_XYZ"]

    # --- isinstance check ---

    def test_isinstance(self):
        """vtkInformation instances should still be vtkInformation."""
        self.assertIsInstance(self.info, vtkInformation)


class TestInformationVector(Testing.vtkTest):
    def setUp(self):
        self.vec = vtkInformationVector()

    def test_len_empty(self):
        self.assertEqual(len(self.vec), 0)

    def test_append_and_len(self):
        self.vec.append(vtkInformation())
        self.vec.append(vtkInformation())
        self.assertEqual(len(self.vec), 2)

    def test_getitem(self):
        info = vtkInformation()
        info[STRING_KEY] = "hello"
        self.vec.append(info)
        self.assertEqual(self.vec[0][STRING_KEY], "hello")

    def test_getitem_negative(self):
        self.vec.append(vtkInformation())
        info = vtkInformation()
        info[INT_KEY] = 99
        self.vec.append(info)
        self.assertEqual(self.vec[-1][INT_KEY], 99)

    def test_getitem_out_of_range(self):
        with self.assertRaises(IndexError):
            _ = self.vec[0]

    def test_setitem(self):
        self.vec.append(vtkInformation())
        replacement = vtkInformation()
        replacement[INT_KEY] = 42
        self.vec[0] = replacement
        self.assertEqual(self.vec[0][INT_KEY], 42)

    def test_setitem_out_of_range(self):
        with self.assertRaises(IndexError):
            self.vec[0] = vtkInformation()

    def test_delitem(self):
        self.vec.append(vtkInformation())
        self.vec.append(vtkInformation())
        del self.vec[0]
        self.assertEqual(len(self.vec), 1)

    def test_delitem_negative(self):
        self.vec.append(vtkInformation())
        self.vec.append(vtkInformation())
        del self.vec[-1]
        self.assertEqual(len(self.vec), 1)

    def test_delitem_out_of_range(self):
        with self.assertRaises(IndexError):
            del self.vec[0]

    def test_iter(self):
        for _ in range(3):
            self.vec.append(vtkInformation())
        items = list(self.vec)
        self.assertEqual(len(items), 3)
        self.assertIsInstance(items[0], vtkInformation)

    def test_contains(self):
        info = vtkInformation()
        self.assertNotIn(info, self.vec)
        self.vec.append(info)
        self.assertIn(info, self.vec)

    def test_slice(self):
        for i in range(4):
            info = vtkInformation()
            info[INT_KEY] = i
            self.vec.append(info)
        sliced = self.vec[1:3]
        self.assertEqual(len(sliced), 2)
        self.assertEqual(sliced[0][INT_KEY], 1)
        self.assertEqual(sliced[1][INT_KEY], 2)

    def test_repr(self):
        self.vec.append(vtkInformation())
        r = repr(self.vec)
        self.assertIn("vtkInformationVector", r)

    def test_isinstance(self):
        self.assertIsInstance(self.vec, vtkInformationVector)


if __name__ == "__main__":
    Testing.main([(TestInformationDict, "test"),
                  (TestInformationVector, "test")])
