"""Pythonic interface for vtkMatrix3x3 and vtkMatrix4x4.

Adds natural indexing, arithmetic, and comparison operators::

    m = vtkMatrix4x4([[1,0,0,1],[0,1,0,2],[0,0,1,3],[0,0,0,1]])
    m[0, 3]            # 1.0
    m[0, 3] = 5.0      # SetElement(0, 3, 5.0)
    m[1]               # array([1., 0., 0., 0.]) — row as numpy array
    m[1] = [0,0,0,0]   # set entire row
    m[1:3]             # 2x4 numpy array (row slice)
    m[0, 1:3]          # array([0., 0.]) (column slice of a row)
    m[1:3, 2]          # array([0., 0.]) (row slice of a column)
    m[1:3, 1:3]        # 2x2 numpy submatrix
    len(m)             # 4
    result = m @ other  # matrix multiply
    inv = ~m            # invert
"""

import numpy as np

from vtkmodules.vtkCommonMath import vtkMatrix3x3, vtkMatrix4x4


class _MatrixMixin:
    """Shared implementation for vtkMatrix3x3 and vtkMatrix4x4 overrides."""

    def __init__(self, *args, **kwargs):
        # SWIG pointer reconstruction: tp_new already returned the
        # existing object; skip mixin init to avoid clobbering state.
        if args and isinstance(args[0], str):
            return
        super().__init__(**kwargs)
        if args:
            self._init_from_data(args[0])

    def _init_from_data(self, data):
        """Populate from nested list/tuple or flat sequence."""
        n = self._size
        if hasattr(data, '__len__') and len(data) == n:
            first = data[0]
            if hasattr(first, '__len__') and len(first) == n:
                # Nested: [[row0], [row1], ...]
                for i in range(n):
                    row = data[i]
                    if len(row) != n:
                        raise ValueError(
                            "Row %d has %d elements, expected %d" % (i, len(row), n))
                    for j in range(n):
                        self.SetElement(i, j, float(row[j]))
                return
        # Flat sequence of n*n elements
        flat = list(data)
        if len(flat) != n * n:
            raise ValueError(
                "Expected %d elements for %dx%d matrix, got %d"
                % (n * n, n, n, len(flat)))
        for i in range(n):
            for j in range(n):
                self.SetElement(i, j, float(flat[i * n + j]))

    # ---- indexing ------------------------------------------------------------
    def _normalize_index(self, index, axis="row"):
        n = self._size
        if index < 0:
            index += n
        if index < 0 or index >= n:
            raise IndexError(
                "%s index %d out of range for %dx%d matrix" % (axis, index, n, n))
        return index

    def _resolve_indices(self, key):
        """Return (rows, cols) where each is an int or list of ints."""
        n = self._size
        if not isinstance(key, tuple):
            key = (key,)
        if len(key) == 1:
            rows = self._to_indices(key[0], "row")
            cols = list(range(n))
            # Single int row → cols is "all", but we still need to know
            # whether to squeeze the row dimension.
            return rows, cols, not isinstance(key[0], slice)
        if len(key) == 2:
            rows = self._to_indices(key[0], "row")
            cols = self._to_indices(key[1], "column")
            row_scalar = not isinstance(key[0], slice)
            col_scalar = not isinstance(key[1], slice)
            return rows, cols, row_scalar, col_scalar
        raise IndexError("matrix index must be [i, j] or [i]")

    def _to_indices(self, key, axis):
        n = self._size
        if isinstance(key, slice):
            return list(range(*key.indices(n)))
        return [self._normalize_index(key, axis)]

    def __getitem__(self, key):
        if not isinstance(key, tuple):
            key = (key,)

        n = self._size

        if len(key) == 1:
            k = key[0]
            if isinstance(k, slice):
                rows = list(range(*k.indices(n)))
                if not rows:
                    return np.empty((0, n), dtype=np.float64)
                return np.array(
                    [[self.GetElement(i, j) for j in range(n)] for i in rows],
                    dtype=np.float64,
                )
            # Single int → row as 1D array
            i = self._normalize_index(k, "row")
            return np.array(
                [self.GetElement(i, j) for j in range(n)], dtype=np.float64
            )

        if len(key) != 2:
            raise IndexError("matrix index must be [i, j] or [i]")

        row_key, col_key = key
        row_scalar = not isinstance(row_key, slice)
        col_scalar = not isinstance(col_key, slice)
        rows = self._to_indices(row_key, "row")
        cols = self._to_indices(col_key, "column")

        if row_scalar and col_scalar:
            # m[i, j] → scalar
            return self.GetElement(rows[0], cols[0])

        data = [[self.GetElement(i, j) for j in cols] for i in rows]
        result = np.array(data, dtype=np.float64)

        # Squeeze scalar dimensions
        if row_scalar:
            result = result[0]
        elif col_scalar:
            result = result[:, 0]

        return result

    def __setitem__(self, key, value):
        if not isinstance(key, tuple):
            key = (key,)

        n = self._size

        if len(key) == 1:
            k = key[0]
            if isinstance(k, slice):
                rows = list(range(*k.indices(n)))
                values = np.asarray(value, dtype=np.float64)
                if values.ndim == 1:
                    # Setting all selected rows to the same row
                    if len(values) != n:
                        raise ValueError(
                            "row must have %d elements, got %d" % (n, len(values)))
                    for i in rows:
                        for j in range(n):
                            self.SetElement(i, j, float(values[j]))
                else:
                    if values.shape != (len(rows), n):
                        raise ValueError(
                            "expected shape (%d, %d), got %s"
                            % (len(rows), n, values.shape))
                    for ri, i in enumerate(rows):
                        for j in range(n):
                            self.SetElement(i, j, float(values[ri, j]))
                return
            # Single int → set entire row
            i = self._normalize_index(k, "row")
            row = list(value)
            if len(row) != n:
                raise ValueError(
                    "row must have %d elements, got %d" % (n, len(row)))
            for j in range(n):
                self.SetElement(i, j, float(row[j]))
            return

        if len(key) != 2:
            raise IndexError("matrix index must be [i, j] or [i]")

        row_key, col_key = key
        row_scalar = not isinstance(row_key, slice)
        col_scalar = not isinstance(col_key, slice)
        rows = self._to_indices(row_key, "row")
        cols = self._to_indices(col_key, "column")

        if row_scalar and col_scalar:
            # m[i, j] = v
            self.SetElement(rows[0], cols[0], float(value))
            return

        values = np.asarray(value, dtype=np.float64)

        if row_scalar:
            # m[i, slice] = 1D
            if values.shape != (len(cols),):
                raise ValueError(
                    "expected %d elements, got %s" % (len(cols), values.shape))
            for ci, j in enumerate(cols):
                self.SetElement(rows[0], j, float(values[ci]))
        elif col_scalar:
            # m[slice, j] = 1D
            if values.shape != (len(rows),):
                raise ValueError(
                    "expected %d elements, got %s" % (len(rows), values.shape))
            for ri, i in enumerate(rows):
                self.SetElement(i, cols[0], float(values[ri]))
        else:
            # m[slice, slice] = 2D
            if values.shape != (len(rows), len(cols)):
                raise ValueError(
                    "expected shape (%d, %d), got %s"
                    % (len(rows), len(cols), values.shape))
            for ri, i in enumerate(rows):
                for ci, j in enumerate(cols):
                    self.SetElement(i, j, float(values[ri, ci]))

    # ---- protocols -----------------------------------------------------------
    def __len__(self):
        return self._size

    def __eq__(self, other):
        if not isinstance(other, type(self).__mro__[2]):  # base VTK class
            return NotImplemented
        n = self._size
        for i in range(n):
            for j in range(n):
                if self.GetElement(i, j) != other.GetElement(i, j):
                    return False
        return True

    def __ne__(self, other):
        result = self.__eq__(other)
        if result is NotImplemented:
            return result
        return not result

    def __matmul__(self, other):
        if not isinstance(other, type(self).__mro__[2]):
            return NotImplemented
        result = type(self)()
        self._multiply(self, other, result)
        return result

    def __invert__(self):
        result = type(self)()
        self.Invert(self, result)
        return result

    def __repr__(self):
        n = self._size
        cls = type(self).__mro__[2].__name__
        rows = []
        for i in range(n):
            row = [self.GetElement(i, j) for j in range(n)]
            rows.append(repr(row))
        return "%s([%s])" % (cls, ", ".join(rows))


@vtkMatrix3x3.override
class Matrix3x3(_MatrixMixin, vtkMatrix3x3):
    _size = 3
    _multiply = staticmethod(vtkMatrix3x3.Multiply3x3)


@vtkMatrix4x4.override
class Matrix4x4(_MatrixMixin, vtkMatrix4x4):
    _size = 4
    _multiply = staticmethod(vtkMatrix4x4.Multiply4x4)
