import vtkmodules.test.Testing as vtkTesting
import vtkmodules.numpy_interface.algorithms
import vtkmodules.numpy_interface.dataset_adapter as dsa
from typing import Tuple
import numpy as np

NUMPY_FUNCTIONS = [
    lambda a: a + a,
    lambda a: a - a,
    lambda a: a * a,
    lambda a: a / (a + 1),
    lambda a: np.cosh(a),
    lambda a: np.arcsinh(a),
    lambda a: np.heaviside(a, 2),
    lambda a: np.bitwise_or(a.astype(int), 1),
    lambda a: np.logical_not(a),
    lambda a: np.sum(a, axis=0),
    lambda a: np.sum(a, axis=1),
    lambda a: np.max(a, axis=0),
    lambda a: np.max(a, axis=1),
    lambda a: np.min(a, axis=0),
    lambda a: np.min(a, axis=1),
    lambda a: np.mean(a, axis=0),
    lambda a: np.mean(a, axis=1),
    lambda a: np.all(a < 3, axis=0),
    lambda a: np.all(a < 3, axis=1),
    lambda a: np.var(a, axis=0),
    lambda a: np.var(a, axis=1),
    lambda a: np.std(a, axis=0),
    lambda a: np.std(a, axis=1),
    lambda a: np.isin(a, np.array([0, 1, 2])),
    lambda a: np.sqrt(a),
    lambda a: np.isnan(a),
    lambda a: np.negative(a),
    lambda a: np.reciprocal(a + 1),
    lambda a: np.square(a),
    lambda a: np.exp(a),
    lambda a: np.floor(a),
    lambda a: np.ceil(a),
    lambda a: np.rint(a),
    lambda a: np.sin(a),
    lambda a: np.cos(a),
    lambda a: np.tan(a),
    lambda a: np.arcsin(a/10),
    lambda a: np.arccos(a/10),
    lambda a: np.arctan(a),
    lambda a: np.arctan2(a, a+1),
    lambda a: np.sinh(a),
    lambda a: np.cosh(a),
    lambda a: np.tanh(a),
    lambda a: np.arcsinh(a),
    lambda a: np.arccosh(a + 1),
    lambda a: np.arctanh(a/10),
    lambda a: np.flatnonzero(a),
    lambda a: np.expand_dims(a, 1),
    lambda a: np.power(a, 5),
    lambda a: np.power(a, a),
    lambda a: np.power(a, np.array([[2,2,2],[2,2,2],[2,2,2],[2,2,2],[2,2,2],[2,2,2],[2,2,2]])),
    lambda a: np.shape(a),
    lambda a: len(a),
    lambda a: 1 in a,
    lambda a: [0, 2, 4] in a,
    lambda a: [0, 4, 0] in a,
    lambda a: np.array(a)
]

SLICING = [
    2,
    5,
    (0, 2),
    (-1, 0),
    (2, -1),
    (4, -2),
    slice(None),
    slice(0, 3),
    slice(3, None),
    slice(-3, None),
    slice(1, 6, 1),
    slice(1, 6, 2),
    slice(1, 6, 3),
    slice(-6, -2),
    slice(5, 2, -1),
    slice(6, 0, -2),
    slice(6, 1, -3),
    (slice(None), slice(0, 2)),
    (slice(1, 5), slice(1, None)),
    (slice(None), 0),
    (slice(None), -1),
    (2, slice(None)),
    (-4, slice(1, 3, -1)),
    (slice(2, 5), slice(-2, -1)),
    (slice(None, None, -1), slice(None)),
    (slice(None), slice(None, None, -1)),
    np.array([0, 3, 6]),
    np.array([0, 1, 2, 5, 3]),
    np.array(-4),
    [-3],
    [5, 2, 3],
]

MISC_FUNCTIONS = [
    lambda a: reversed(a),
    lambda a: iter(a),
    lambda a: np.zeros_like(a),
    lambda a: np.ones_like(a)
]

SET_INDICES = [
    (5, 1),
    ((3, 2), 17),
    (slice(1, 3), 9),
    ((slice(2, 4), 1), -3),
    (np.array([0, 1, 2, 5, 3]), 42),
    (np.array(0), 8)
]

class TestVTKCompositeDataArray(vtkTesting.vtkTest):
    def get_arrays(self) -> Tuple[dsa.VTKCompositeDataArray, np.ndarray]:
        chunk1 = np.array([[1, 0, 0],
                           [2, 0, 0]])
        chunk2 = np.array([[0, 3, 0],
                           [0, 4, 0],
                           [0, 5, 0],
                           [0, 6, 0]])
        chunk3 = dsa.NoneArray
        chunk4 = np.array([[0, 0, 7]])
        composite = dsa.VTKCompositeDataArray([dsa.VTKArray(chunk1), dsa.VTKArray(chunk2), chunk3, dsa.VTKArray(chunk4)])
        array = np.array([[1, 0, 0], [2, 0, 0], [0, 3, 0], [0, 4, 0], [0, 5, 0], [0, 6, 0], [0, 0, 7]])
        return composite, array

    def _to_np(self, array) -> np.ndarray:
        if isinstance(array, dsa.VTKCompositeDataArray):
            array = np.concatenate([np.asarray(a) for a in array.Arrays if a is not dsa.NoneArray], axis=0)
        return array

    def test_numpy_functions(self) -> None:
        composite, np_array = self.get_arrays()
        for func in NUMPY_FUNCTIONS:
            composite_transformed = func(composite)
            np_array_transformed = func(np_array)
            np_composite_transformed = self._to_np(composite_transformed)
            for v1, v2 in zip(np.nditer(np_array_transformed), np.nditer(np_composite_transformed)):
                self.assertAlmostEqual(v1 , v2)

    def test_slicing(self) -> None:
        composite, np_array = self.get_arrays()
        for slc in SLICING:
            composite_sliced = composite[slc]
            np_array_sliced = np_array[slc]
            np_composite_sliced = self._to_np(composite_sliced)
            self.assertEqual(np.all(np_array_sliced == np_composite_sliced) , True)

    def test_composite_get_set(self) -> None:
        composite, np_array = self.get_arrays()
        composite[composite > 6] = -1
        np_array[np_array > 6] = -1
        np_composite = self._to_np(composite)
        self.assertEqual(np.all(np_composite == np_array), True)
        composite_transformed = composite[composite > 2]
        np_array_transformed = np_array[np_array > 2]
        np_composite_transformed = self._to_np(composite_transformed)
        self.assertEqual(np.all(np_composite_transformed == np_array_transformed), True)

    def test_misc(self) -> None:
        composite, np_array = self.get_arrays()
        for func in MISC_FUNCTIONS:
            composite_transformed = func(composite)
            np_array_transformed = func(np_array)
            for (v1, v2) in zip(composite_transformed, np_array_transformed):
                self.assertEqual(np.all(v1 == v2) , True)

    def test_set(self) -> None:
        composite, np_array = self.get_arrays()
        for slc, value in SET_INDICES:
            composite[slc] = value
            np_array[slc] = value
            np_composite = self._to_np(composite)
            self.assertEqual(np.all(np_composite == np_array), True)

if __name__ == "__main__":
    vtkTesting.main([(TestVTKCompositeDataArray, "test")])
