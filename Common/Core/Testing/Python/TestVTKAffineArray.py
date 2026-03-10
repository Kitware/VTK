# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""Test VTKAffineArray numpy-compatible mixin."""
import sys
import numpy as np
from vtkmodules.vtkCommonCore import vtkAffineArray
from vtkmodules.numpy_interface.vtk_affine_array import VTKAffineArray


def make_affine(n, slope, intercept, dtype='float64', ncomp=1):
    """Helper to create an affine array."""
    if ncomp == 1:
        return vtkAffineArray[dtype](n, slope, intercept)
    arr = vtkAffineArray[dtype]((n, ncomp), slope, intercept)
    return arr


def test_basic_creation():
    """Test that vtkAffineArray is automatically a VTKAffineArray mixin."""
    arr = make_affine(100, 2.0, 5.0)

    assert isinstance(arr, VTKAffineArray), \
        f"Expected VTKAffineArray, got {type(arr).__name__}"
    assert arr._slope == 2.0, f"Expected slope 2.0, got {arr._slope}"
    assert arr._intercept == 5.0, f"Expected intercept 5.0, got {arr._intercept}"
    assert arr._num_values == 100, f"Expected 100 values, got {arr._num_values}"

    return True


def test_shape_dtype():
    """Test shape, dtype, ndim, size, and len properties."""
    arr = make_affine(50, 1.0, 0.0, dtype='float32')

    assert arr.shape == (50,), f"Expected shape (50,), got {arr.shape}"
    assert arr.dtype == np.float32, f"Expected float32, got {arr.dtype}"
    assert arr.ndim == 1, f"Expected ndim 1, got {arr.ndim}"
    assert arr.size == 50, f"Expected size 50, got {arr.size}"
    assert len(arr) == 50, f"Expected len 50, got {len(arr)}"

    return True


def test_materialization():
    """Test that np.array(arr) produces correct values."""
    arr = make_affine(10, 3.0, 10.0)

    materialized = np.array(arr)
    expected = np.array([10, 13, 16, 19, 22, 25, 28, 31, 34, 37], dtype=np.float64)
    np.testing.assert_array_almost_equal(materialized, expected)

    return True


def test_single_element_indexing():
    """Test single element indexing without materialization."""
    arr = make_affine(100, 2.0, 1.0)

    assert arr[0] == 1.0, f"arr[0] = {arr[0]}, expected 1.0"
    assert arr[1] == 3.0, f"arr[1] = {arr[1]}, expected 3.0"
    assert arr[49] == 99.0, f"arr[49] = {arr[49]}, expected 99.0"
    assert arr[-1] == 199.0, f"arr[-1] = {arr[-1]}, expected 199.0"
    assert arr[-2] == 197.0, f"arr[-2] = {arr[-2]}, expected 197.0"

    return True


def test_slice_indexing():
    """Test slice indexing (falls back to materialization)."""
    arr = make_affine(10, 1.0, 0.0)

    sliced = arr[2:5]
    expected = np.array([2, 3, 4], dtype=np.float64)
    np.testing.assert_array_almost_equal(sliced, expected)

    return True


def test_scalar_multiplication():
    """Test that scalar multiplication stays lazy."""
    arr = make_affine(100, 2.0, 5.0)
    result = arr * 3

    assert isinstance(result, VTKAffineArray), \
        f"Expected VTKAffineArray, got {type(result)}"
    assert result._slope == 6.0, f"Expected slope 6.0, got {result._slope}"
    assert result._intercept == 15.0, f"Expected intercept 15.0, got {result._intercept}"

    np.testing.assert_array_almost_equal(
        np.array(result)[:5],
        np.array([15, 21, 27, 33, 39], dtype=np.float64)
    )

    return True


def test_scalar_addition():
    """Test that scalar addition stays lazy."""
    arr = make_affine(100, 2.0, 5.0)
    result = arr + 10

    assert isinstance(result, VTKAffineArray)
    assert result._slope == 2.0, "Slope should be unchanged"
    assert result._intercept == 15.0, f"Expected intercept 15.0, got {result._intercept}"

    return True


def test_scalar_subtraction():
    """Test scalar subtraction stays lazy."""
    arr = make_affine(100, 2.0, 10.0)

    result1 = arr - 5
    assert isinstance(result1, VTKAffineArray)
    assert result1._intercept == 5.0

    result2 = 100 - arr
    assert isinstance(result2, VTKAffineArray)
    assert result2._slope == -2.0
    assert result2._intercept == 90.0

    return True


def test_scalar_division():
    """Test scalar division stays lazy."""
    arr = make_affine(100, 4.0, 8.0)
    result = arr / 2

    assert isinstance(result, VTKAffineArray)
    assert result._slope == 2.0
    assert result._intercept == 4.0

    return True


def test_negation():
    """Test unary negation stays lazy."""
    arr = make_affine(100, 3.0, 7.0)
    result = -arr

    assert isinstance(result, VTKAffineArray)
    assert result._slope == -3.0
    assert result._intercept == -7.0

    return True


def test_sum_closed_form():
    """Test that np.sum uses closed-form formula."""
    arr = make_affine(100, 1.0, 0.0)
    result = np.sum(arr)
    assert result == 4950.0, f"Expected 4950.0, got {result}"

    arr2 = make_affine(100, 2.0, 5.0)
    result2 = np.sum(arr2)
    assert result2 == 10400.0, f"Expected 10400.0, got {result2}"

    return True


def test_mean_closed_form():
    """Test that np.mean uses closed-form formula."""
    arr = make_affine(100, 1.0, 0.0)
    result = np.mean(arr)
    assert abs(result - 49.5) < 1e-10, f"Expected 49.5, got {result}"

    return True


def test_min_max_closed_form():
    """Test that np.min and np.max use closed-form formulas."""
    arr1 = make_affine(10, 2.0, 10.0)
    assert np.min(arr1) == 10.0
    assert np.max(arr1) == 28.0

    arr2 = make_affine(10, -3.0, 100.0)
    assert np.min(arr2) == 73.0
    assert np.max(arr2) == 100.0

    return True


def test_std_closed_form():
    """Test that np.std uses closed-form formula."""
    arr = make_affine(100, 1.0, 0.0)

    materialized = np.array(arr)
    expected_std = np.std(materialized)
    result_std = np.std(arr)

    assert abs(result_std - expected_std) < 1e-10, \
        f"std mismatch: expected {expected_std}, got {result_std}"

    return True


def test_chained_operations():
    """Test chained lazy operations."""
    arr = make_affine(100, 1.0, 0.0)

    result = (arr * 2 + 10) / 2

    assert isinstance(result, VTKAffineArray)
    assert result._slope == 1.0
    assert result._intercept == 5.0
    assert result[0] == 5.0
    assert result[1] == 6.0
    assert result[99] == 104.0

    return True


def test_getslope_getintercept_methods():
    """Test that GetSlope and GetIntercept methods are accessible."""
    arr = make_affine(100, 3.5, 7.25)

    assert arr.GetSlope() == 3.5, f"GetSlope returned {arr.GetSlope()}"
    assert arr.GetIntercept() == 7.25, f"GetIntercept returned {arr.GetIntercept()}"

    return True


def test_multi_component():
    """Test array with multiple components."""
    arr = make_affine(10, 1.0, 0.0, ncomp=3)

    assert arr.shape == (10, 3), f"Expected shape (10, 3), got {arr.shape}"
    assert arr.ndim == 2
    assert arr.size == 30

    materialized = np.array(arr)
    assert materialized.shape == (10, 3)
    np.testing.assert_array_almost_equal(materialized[0], [0, 1, 2])

    return True


def test_from_params_constructor():
    """Test creating VTKAffineArray from parameters."""
    arr = VTKAffineArray._from_params(
        slope=2.0, intercept=10.0, num_values=50, dtype=np.float64)

    assert isinstance(arr, VTKAffineArray)
    assert arr._slope == 2.0
    assert arr._intercept == 10.0
    assert arr._num_values == 50

    assert arr[0] == 10.0
    assert arr[1] == 12.0
    assert np.sum(arr) == 50 * 10 + 2 * 50 * 49 / 2

    arr.SetName("test_forwarding")
    assert arr.GetName() == "test_forwarding"
    assert arr.GetNumberOfTuples() == 50
    assert arr.GetSlope() == 2.0
    assert arr.GetIntercept() == 10.0

    return True


def test_dataset_roundtrip():
    """Test that VTKAffineArray survives dataset assignment and retrieval."""
    from vtkmodules.vtkCommonDataModel import vtkImageData

    img = vtkImageData()
    img.SetDimensions(10, 10, 10)
    num_points = img.GetNumberOfPoints()

    original = VTKAffineArray._from_params(
        slope=0.5, intercept=10.0, num_values=num_points, dtype=np.float64)
    assert isinstance(original, VTKAffineArray)

    img.point_data['affine_test'] = original

    retrieved = img.point_data['affine_test']

    assert isinstance(retrieved, VTKAffineArray), \
        f"Expected VTKAffineArray, got {type(retrieved).__name__}"

    assert retrieved._slope == 0.5
    assert retrieved._intercept == 10.0
    assert retrieved._num_values == num_points

    expected_sum = num_points * 10.0 + 0.5 * num_points * (num_points - 1) / 2
    assert np.sum(retrieved) == expected_sum

    return True


# ---- New tests for feature parity ------------------------------------------

def test_nbytes():
    """Test nbytes property."""
    arr = make_affine(100, 1.0, 0.0, dtype='float64')
    assert arr.nbytes == 100 * 8, f"Expected 800, got {arr.nbytes}"

    arr32 = make_affine(100, 1.0, 0.0, dtype='float32')
    assert arr32.nbytes == 100 * 4, f"Expected 400, got {arr32.nbytes}"

    return True


def test_T_property():
    """Test T (transpose) property."""
    arr = make_affine(5, 1.0, 0.0)
    t = arr.T
    np.testing.assert_array_equal(t, np.arange(5, dtype=np.float64))

    return True


def test_str_repr():
    """Test __str__ and __repr__."""
    arr = make_affine(10, 2.0, 5.0)
    s = str(arr)
    assert "VTKAffineArray" in s
    assert "slope=2.0" in s
    r = repr(arr)
    assert "VTKAffineArray" in r

    return True


def test_hash_none():
    """Test that __hash__ is None (since __eq__ is defined)."""
    arr = make_affine(10, 1.0, 0.0)
    try:
        hash(arr)
        assert False, "Should not be hashable"
    except TypeError:
        pass

    return True


def test_setitem_read_only():
    """Test that __setitem__ raises TypeError."""
    arr = make_affine(10, 1.0, 0.0)
    try:
        arr[0] = 42
        assert False, "Should have raised TypeError"
    except TypeError as e:
        assert "read-only" in str(e)

    return True


def test_iter():
    """Test __iter__."""
    arr = make_affine(5, 2.0, 1.0)
    values = list(arr)
    expected = [1.0, 3.0, 5.0, 7.0, 9.0]
    for v, e in zip(values, expected):
        assert v == e, f"Expected {e}, got {v}"

    return True


def test_missing_operators():
    """Test floordiv, pow, mod operators."""
    arr = make_affine(5, 2.0, 1.0)  # [1, 3, 5, 7, 9]
    materialized = np.array(arr)

    # floor division
    result = arr // 2
    np.testing.assert_array_equal(np.asarray(result), materialized // 2)

    # reverse floor division
    result = 10 // arr
    np.testing.assert_array_equal(np.asarray(result), 10 // materialized)

    # power
    result = arr ** 2
    np.testing.assert_array_equal(np.asarray(result), materialized ** 2)

    # reverse power
    result = 2 ** arr
    np.testing.assert_array_equal(np.asarray(result), 2 ** materialized)

    # modulo
    result = arr % 3
    np.testing.assert_array_equal(np.asarray(result), materialized % 3)

    # reverse modulo
    result = 10 % arr
    np.testing.assert_array_equal(np.asarray(result), 10 % materialized)

    return True


def test_comparison_operators():
    """Test comparison operators."""
    arr = make_affine(5, 2.0, 1.0)  # [1, 3, 5, 7, 9]

    result = arr < 5
    expected = np.array([True, True, False, False, False])
    np.testing.assert_array_equal(np.asarray(result), expected)

    result = arr <= 5
    expected = np.array([True, True, True, False, False])
    np.testing.assert_array_equal(np.asarray(result), expected)

    result = arr > 5
    expected = np.array([False, False, False, True, True])
    np.testing.assert_array_equal(np.asarray(result), expected)

    result = arr >= 5
    expected = np.array([False, False, True, True, True])
    np.testing.assert_array_equal(np.asarray(result), expected)

    result = arr == 5
    expected = np.array([False, False, True, False, False])
    np.testing.assert_array_equal(np.asarray(result), expected)

    result = arr != 5
    expected = np.array([True, True, False, True, True])
    np.testing.assert_array_equal(np.asarray(result), expected)

    return True


def test_unary_pos_abs():
    """Test __pos__ and __abs__ operators."""
    arr = make_affine(10, 2.0, 5.0)  # all positive

    # positive
    result = +arr
    assert isinstance(result, VTKAffineArray)
    assert result._slope == 2.0
    assert result._intercept == 5.0

    # abs of all-positive array
    result = abs(arr)
    assert isinstance(result, VTKAffineArray)
    assert result._slope == 2.0

    # abs of all-negative array
    arr_neg = make_affine(10, -2.0, -5.0)
    result = abs(arr_neg)
    assert isinstance(result, VTKAffineArray)
    assert result._slope == 2.0
    assert result._intercept == 5.0

    return True


def test_var_closed_form():
    """Test that np.var uses closed-form formula."""
    arr = make_affine(100, 1.0, 0.0)

    materialized = np.array(arr)
    expected_var = np.var(materialized)
    result_var = np.var(arr)

    assert abs(result_var - expected_var) < 1e-10, \
        f"var mismatch: expected {expected_var}, got {result_var}"

    # test with ddof=1
    expected_var1 = np.var(materialized, ddof=1)
    result_var1 = np.var(arr, ddof=1)
    assert abs(result_var1 - expected_var1) < 1e-8, \
        f"var(ddof=1) mismatch: expected {expected_var1}, got {result_var1}"

    return True


def test_method_reductions():
    """Test method-based reductions (sum, mean, min, max, std, var)."""
    arr = make_affine(100, 1.0, 0.0)
    materialized = np.array(arr)

    assert arr.sum() == materialized.sum()
    assert abs(arr.mean() - materialized.mean()) < 1e-10
    assert arr.min() == materialized.min()
    assert arr.max() == materialized.max()
    assert abs(arr.std() - materialized.std()) < 1e-10
    assert abs(arr.var() - materialized.var()) < 1e-10

    return True


def test_any_all_prod():
    """Test any, all, prod methods."""
    arr = make_affine(5, 1.0, 1.0)  # [1, 2, 3, 4, 5]
    assert arr.any() == True
    assert arr.all() == True
    assert arr.prod() == 120.0

    # Array containing zero
    arr_zero = make_affine(5, 1.0, 0.0)  # [0, 1, 2, 3, 4]
    assert arr_zero.any() == True
    assert arr_zero.all() == False

    return True


def test_argmin_argmax():
    """Test argmin and argmax methods."""
    arr = make_affine(10, 2.0, 5.0)  # positive slope
    assert arr.argmin() == 0
    assert arr.argmax() == 9

    arr_neg = make_affine(10, -3.0, 100.0)  # negative slope
    assert arr_neg.argmin() == 9
    assert arr_neg.argmax() == 0

    return True


def test_cumsum_cumprod():
    """Test cumsum and cumprod methods."""
    arr = make_affine(5, 1.0, 1.0)  # [1, 2, 3, 4, 5]
    materialized = np.array(arr)

    np.testing.assert_array_almost_equal(arr.cumsum(), materialized.cumsum())
    np.testing.assert_array_almost_equal(arr.cumprod(), materialized.cumprod())

    return True


def test_shape_layout_methods():
    """Test reshape, flatten, ravel, copy, squeeze, transpose, tolist."""
    arr = make_affine(6, 1.0, 0.0)
    materialized = np.array(arr)

    # reshape
    reshaped = arr.reshape(2, 3)
    np.testing.assert_array_equal(reshaped, materialized.reshape(2, 3))

    # flatten
    flat = arr.flatten()
    np.testing.assert_array_equal(flat, materialized.flatten())

    # ravel
    rav = arr.ravel()
    np.testing.assert_array_equal(rav, materialized.ravel())

    # copy
    c = arr.copy()
    np.testing.assert_array_equal(c, materialized)

    # squeeze
    sq = arr.squeeze()
    np.testing.assert_array_equal(sq, materialized.squeeze())

    # tolist
    assert arr.tolist() == materialized.tolist()

    return True


def test_clip_round_sort():
    """Test clip, round, sort methods."""
    arr = make_affine(10, 0.7, 0.1)
    materialized = np.array(arr)

    # clip
    clipped = arr.clip(1.0, 5.0)
    np.testing.assert_array_almost_equal(clipped, materialized.clip(1.0, 5.0))

    # round
    rounded = arr.round(0)
    np.testing.assert_array_almost_equal(rounded, materialized.round(0))

    # sort
    arr_neg = make_affine(10, -1.0, 9.0)
    sorted_arr = arr_neg.sort()
    np.testing.assert_array_almost_equal(sorted_arr, np.sort(np.array(arr_neg)))

    return True


def test_dot():
    """Test dot method."""
    arr = make_affine(5, 1.0, 0.0)  # [0, 1, 2, 3, 4]
    other = np.array([1.0, 1.0, 1.0, 1.0, 1.0])
    result = arr.dot(other)
    assert result == 10.0, f"Expected 10.0, got {result}"

    return True


def test_astype():
    """Test astype method."""
    arr = make_affine(10, 1.0, 0.0, dtype='float64')
    result = arr.astype(np.float32)
    assert result.dtype == np.float32

    return True


def test_wrap_result_metadata():
    """Test that _wrap_result propagates metadata through non-lazy operations."""
    from vtkmodules.vtkCommonDataModel import vtkImageData
    from vtkmodules.numpy_interface.vtk_aos_array import VTKAOSArray

    img = vtkImageData()
    img.SetDimensions(6, 1, 1)

    arr = make_affine(5, 2.0, 1.0)  # [1, 3, 5, 7, 9]
    arr._set_dataset(img)
    arr._association = 0  # POINT

    # Non-lazy op (e.g. power) should wrap result with metadata
    result = arr ** 2
    assert isinstance(result, VTKAOSArray), \
        f"Expected VTKAOSArray, got {type(result).__name__}"
    assert result.dataset is img, "Metadata dataset not propagated"
    assert result.association == 0, "Metadata association not propagated"
    np.testing.assert_array_equal(
        np.asarray(result), np.array([1, 9, 25, 49, 81], dtype=np.float64))

    return True


def test_numpy_any_closed_form():
    """Test that numpy.any uses O(1) closed-form."""
    # Non-zero array
    arr = make_affine(5, 1.0, 1.0)  # [1, 2, 3, 4, 5]
    assert np.any(arr) == True, "np.any of non-zero affine should be True"

    # Array with zero slope and non-zero intercept
    arr2 = make_affine(5, 0.0, 5.0)  # [5, 5, 5, 5, 5]
    assert np.any(arr2) == True, "np.any of constant non-zero should be True"

    # All-zero array (slope=0, intercept=0)
    arr3 = make_affine(5, 0.0, 0.0)  # [0, 0, 0, 0, 0]
    assert np.any(arr3) == False, "np.any of all-zero should be False"

    # Array containing zero
    arr4 = make_affine(5, 1.0, 0.0)  # [0, 1, 2, 3, 4]
    assert np.any(arr4) == True, "np.any with one zero should be True"

    return True


def test_numpy_all_closed_form():
    """Test that numpy.all uses O(1) closed-form."""
    # Non-zero array
    arr = make_affine(5, 1.0, 1.0)  # [1, 2, 3, 4, 5]
    assert np.all(arr) == True, "np.all of non-zero affine should be True"

    # Constant zero
    arr2 = make_affine(5, 0.0, 0.0)  # [0, 0, 0, 0, 0]
    assert np.all(arr2) == False, "np.all of all-zero should be False"

    # Constant non-zero
    arr3 = make_affine(5, 0.0, 3.0)  # [3, 3, 3, 3, 3]
    assert np.all(arr3) == True, "np.all of constant non-zero should be True"

    # Contains exactly one zero
    arr4 = make_affine(5, 1.0, 0.0)  # [0, 1, 2, 3, 4]
    assert np.all(arr4) == False, "np.all with zero at index 0 should be False"

    # Contains a zero at interior index: slope=2, intercept=-4 -> [−4, −2, 0, 2, 4]
    arr5 = make_affine(5, 2.0, -4.0)
    assert np.all(arr5) == False, "np.all with zero at index 2 should be False"

    # No zero: slope=2, intercept=1 -> [1, 3, 5, 7, 9]
    arr6 = make_affine(5, 2.0, 1.0)
    assert np.all(arr6) == True, "np.all with no zero should be True"

    return True


def test_numpy_prod_via_function():
    """Test that numpy.prod works via __array_function__."""
    arr = make_affine(5, 1.0, 1.0)  # [1, 2, 3, 4, 5]
    result = np.prod(arr)
    expected = np.prod(np.array(arr))
    assert result == expected, f"np.prod: expected {expected}, got {result}"

    return True


def test_ufunc_out_guard():
    """Test that __array_ufunc__ returns NotImplemented when out is provided."""
    arr = make_affine(5, 1.0, 0.0)
    out = np.empty(5)
    result = arr.__array_ufunc__(np.add, '__call__', arr, 1.0, out=(out,))
    # When out is provided, should return NotImplemented, causing numpy
    # to fall back to its own implementation
    assert result is NotImplemented, f"Expected NotImplemented, got {result}"

    return True


def run_all_tests():
    """Run all tests and report results."""
    tests = [
        ("basic_creation", test_basic_creation),
        ("shape_dtype", test_shape_dtype),
        ("materialization", test_materialization),
        ("single_element_indexing", test_single_element_indexing),
        ("slice_indexing", test_slice_indexing),
        ("scalar_multiplication", test_scalar_multiplication),
        ("scalar_addition", test_scalar_addition),
        ("scalar_subtraction", test_scalar_subtraction),
        ("scalar_division", test_scalar_division),
        ("negation", test_negation),
        ("sum_closed_form", test_sum_closed_form),
        ("mean_closed_form", test_mean_closed_form),
        ("min_max_closed_form", test_min_max_closed_form),
        ("std_closed_form", test_std_closed_form),
        ("chained_operations", test_chained_operations),
        ("getslope_getintercept_methods", test_getslope_getintercept_methods),
        ("multi_component", test_multi_component),
        ("from_params_constructor", test_from_params_constructor),
        ("dataset_roundtrip", test_dataset_roundtrip),
        ("nbytes", test_nbytes),
        ("T_property", test_T_property),
        ("str_repr", test_str_repr),
        ("hash_none", test_hash_none),
        ("setitem_read_only", test_setitem_read_only),
        ("iter", test_iter),
        ("missing_operators", test_missing_operators),
        ("comparison_operators", test_comparison_operators),
        ("unary_pos_abs", test_unary_pos_abs),
        ("var_closed_form", test_var_closed_form),
        ("method_reductions", test_method_reductions),
        ("any_all_prod", test_any_all_prod),
        ("argmin_argmax", test_argmin_argmax),
        ("cumsum_cumprod", test_cumsum_cumprod),
        ("shape_layout_methods", test_shape_layout_methods),
        ("clip_round_sort", test_clip_round_sort),
        ("dot", test_dot),
        ("astype", test_astype),
        ("wrap_result_metadata", test_wrap_result_metadata),
        ("numpy_any_closed_form", test_numpy_any_closed_form),
        ("numpy_all_closed_form", test_numpy_all_closed_form),
        ("numpy_prod_via_function", test_numpy_prod_via_function),
        ("ufunc_out_guard", test_ufunc_out_guard),
    ]

    passed = 0
    failed = 0

    for name, test_func in tests:
        try:
            if test_func():
                print(f"PASSED: {name}")
                passed += 1
            else:
                print(f"FAILED: {name}")
                failed += 1
        except Exception as e:
            print(f"FAILED: {name} - {e}")
            import traceback
            traceback.print_exc()
            failed += 1

    print(f"\nResults: {passed} passed, {failed} failed")
    return failed == 0


if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)
