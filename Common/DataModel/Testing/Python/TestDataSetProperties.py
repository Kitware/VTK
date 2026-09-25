"""Tests for point_data, cell_data, and field_data property setters on DataSet."""

import unittest

import vtkmodules.test.Testing as vtkTesting

try:
    import numpy as np
except ImportError:
    print("This test requires numpy!")
    vtkTesting.skip()

from vtkmodules.vtkCommonCore import vtkFloatArray, vtkIntArray, vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkImageData,
    vtkPolyData,
    vtkRectilinearGrid,
    vtkUnstructuredGrid,
)

try:
    import pandas as pd
    HAS_PANDAS = True
except ImportError:
    HAS_PANDAS = False

try:
    import xarray as xr
    HAS_XARRAY = True
except ImportError:
    HAS_XARRAY = False


class TestImageDataProperties(unittest.TestCase):
    """Test point_data/cell_data/field_data setters on vtkImageData."""

    def setUp(self):
        self.img = vtkImageData()
        self.img.SetDimensions(3, 4, 5)
        self.npts = self.img.GetNumberOfPoints()   # 60
        self.ncells = self.img.GetNumberOfCells()   # 24

    def test_point_data_setter_numpy(self):
        """Assign point arrays from a dict of numpy arrays."""
        temp = np.arange(self.npts, dtype=np.float64)
        vel = np.random.rand(self.npts, 3)
        self.img.point_data = {"temperature": temp, "velocity": vel}

        self.assertEqual(self.img.point_data.keys(), ("temperature", "velocity"))
        self.assertEqual(
            self.img.point_data["temperature"].GetNumberOfTuples(), self.npts
        )
        self.assertEqual(
            self.img.point_data["velocity"].GetNumberOfComponents(), 3
        )

    def test_cell_data_setter_numpy(self):
        """Assign cell arrays from a dict of numpy arrays."""
        mat = np.ones(self.ncells, dtype=np.int32)
        self.img.cell_data = {"material": mat}

        self.assertEqual(self.img.cell_data.keys(), ("material",))
        self.assertEqual(
            self.img.cell_data["material"].GetNumberOfTuples(), self.ncells
        )

    def test_field_data_setter_numpy(self):
        """Assign field arrays from a dict of numpy arrays."""
        self.img.field_data = {"info": np.array([1.0, 2.0, 3.0])}

        self.assertEqual(self.img.field_data.keys(), ("info",))
        self.assertEqual(
            self.img.field_data["info"].GetNumberOfTuples(), 3
        )

    def test_setter_with_vtk_array(self):
        """Values can be VTK data arrays instead of numpy arrays."""
        vtk_arr = vtkFloatArray()
        vtk_arr.SetNumberOfTuples(self.npts)
        vtk_arr.FillComponent(0, 42.0)

        self.img.point_data = {"scalar": vtk_arr}

        self.assertEqual(self.img.point_data.keys(), ("scalar",))
        self.assertAlmostEqual(
            self.img.point_data["scalar"].GetValue(0), 42.0
        )

    def test_setter_mixed_vtk_and_numpy(self):
        """Mix VTK arrays and numpy arrays in the same dict."""
        vtk_arr = vtkIntArray()
        vtk_arr.SetNumberOfTuples(self.npts)
        vtk_arr.FillComponent(0, 7)
        np_arr = np.zeros(self.npts, dtype=np.float64)

        self.img.point_data = {"vtk_field": vtk_arr, "np_field": np_arr}

        self.assertEqual(
            set(self.img.point_data.keys()), {"vtk_field", "np_field"}
        )

    def test_setter_replaces_existing(self):
        """Setting point_data clears previous arrays."""
        self.img.point_data = {"old": np.zeros(self.npts)}
        self.assertIn("old", self.img.point_data.keys())

        self.img.point_data = {"new": np.ones(self.npts)}
        self.assertEqual(self.img.point_data.keys(), ("new",))
        self.assertNotIn("old", self.img.point_data.keys())

    def test_getter_unchanged(self):
        """Getter still returns a DataSetAttributes with dict-like access."""
        self.img.point_data = {"x": np.arange(self.npts, dtype=np.float32)}
        pd = self.img.point_data
        self.assertIn("x", pd)
        self.assertEqual(len(pd), 1)

    def test_values_roundtrip(self):
        """Numpy array values survive a set/get roundtrip."""
        original = np.linspace(0, 1, self.npts)
        self.img.point_data = {"data": original}
        retrieved = self.img.point_data["data"]
        np.testing.assert_allclose(np.asarray(retrieved), original)

    def test_point_data_setter_sequence(self):
        """Lists, tuples and ranges are arrays, not scalars."""
        self.img.point_data["list"] = list(range(self.npts))
        self.img.point_data["tuple"] = tuple(0.5 * i for i in range(self.npts))
        self.img.point_data["range"] = range(self.npts)
        self.img.point_data["nested"] = [[i, 0, 0] for i in range(self.npts)]

        np.testing.assert_array_equal(
            np.asarray(self.img.point_data["list"]), np.arange(self.npts)
        )
        np.testing.assert_array_equal(
            np.asarray(self.img.point_data["tuple"]),
            0.5 * np.arange(self.npts),
        )
        np.testing.assert_array_equal(
            np.asarray(self.img.point_data["range"]), np.arange(self.npts)
        )
        self.assertEqual(self.img.point_data["nested"].shape, (self.npts, 3))

    def test_point_data_setter_tuple_broadcast(self):
        """A single tuple fills every point, as a 1-D numpy array does."""
        self.img.point_data["v"] = [1.0, 2.0, 3.0]
        v = np.asarray(self.img.point_data["v"])
        self.assertEqual(v.shape, (self.npts, 3))
        np.testing.assert_array_equal(v[-1], [1.0, 2.0, 3.0])

    def test_cell_data_setter_sequence(self):
        """Assign cell arrays from a list, and from a dict of lists."""
        self.img.cell_data["material"] = [i % 3 for i in range(self.ncells)]
        np.testing.assert_array_equal(
            np.asarray(self.img.cell_data["material"]),
            np.arange(self.ncells) % 3,
        )

        self.img.cell_data = {"id": list(range(self.ncells))}
        self.assertEqual(self.img.cell_data.keys(), ("id",))
        self.assertEqual(
            self.img.cell_data["id"].GetNumberOfTuples(), self.ncells
        )

    def test_field_data_setter_sequence(self):
        """Field data takes its length from a list."""
        self.img.field_data["info"] = [1.0, 2.0, 3.0]
        np.testing.assert_array_equal(
            np.asarray(self.img.field_data["info"]), [1.0, 2.0, 3.0]
        )

    def test_field_data_setter_scalar(self):
        """Scalars, numpy scalars and 0-d arrays make one-value arrays."""
        self.img.field_data["py"] = 0.5
        self.img.field_data["np"] = np.float32(0.5)
        self.img.field_data["zero_d"] = np.array(0.5)
        expected = {"py": np.float64, "np": np.float32, "zero_d": np.float64}
        for name, dtype in expected.items():
            arr = self.img.field_data[name]
            self.assertEqual(arr.GetNumberOfTuples(), 1)
            self.assertEqual(arr.dtype, dtype)
            self.assertEqual(arr[0], 0.5)


class TestPolyDataProperties(unittest.TestCase):
    """Verify setters work on PolyData (inherited from DataSet)."""

    def setUp(self):
        self.pd = vtkPolyData()
        pts = vtkPoints()
        for i in range(10):
            pts.InsertNextPoint(float(i), 0.0, 0.0)
        self.pd.SetPoints(pts)
        self.npts = self.pd.GetNumberOfPoints()

    def test_point_data_setter(self):
        self.pd.point_data = {"ids": np.arange(self.npts)}
        self.assertEqual(self.pd.point_data.keys(), ("ids",))

    def test_point_data_replaces(self):
        self.pd.point_data = {"a": np.zeros(self.npts)}
        self.pd.point_data = {"b": np.ones(self.npts)}
        self.assertEqual(self.pd.point_data.keys(), ("b",))


class TestRectilinearGridProperties(unittest.TestCase):
    """Verify setters work on RectilinearGrid."""

    def setUp(self):
        from vtkmodules.util.numpy_support import numpy_to_vtk

        self.rg = vtkRectilinearGrid()
        x = numpy_to_vtk(np.array([0.0, 1.0, 2.0]))
        y = numpy_to_vtk(np.array([0.0, 1.0, 2.0, 3.0]))
        z = numpy_to_vtk(np.array([0.0, 1.0]))
        self.rg.SetDimensions(3, 4, 2)
        self.rg.SetXCoordinates(x)
        self.rg.SetYCoordinates(y)
        self.rg.SetZCoordinates(z)
        self.npts = self.rg.GetNumberOfPoints()
        self.ncells = self.rg.GetNumberOfCells()

    def test_point_data_setter(self):
        self.rg.point_data = {"temp": np.arange(self.npts, dtype=np.float64)}
        self.assertEqual(self.rg.point_data.keys(), ("temp",))

    def test_cell_data_setter(self):
        self.rg.cell_data = {"mat": np.ones(self.ncells, dtype=np.int32)}
        self.assertEqual(self.rg.cell_data.keys(), ("mat",))


class TestConstructor(unittest.TestCase):
    """Test that point_data/cell_data work as constructor kwargs."""

    def test_image_data_constructor(self):
        """Build an ImageData entirely from keyword arguments."""
        npts = 3 * 4 * 5  # 60
        ncells = 2 * 3 * 4  # 24
        img = vtkImageData(
            dimensions=(3, 4, 5),
            spacing=(0.1, 0.2, 0.3),
            origin=(1.0, 2.0, 3.0),
            point_data={"temperature": np.arange(npts, dtype=np.float64)},
            cell_data={"material": np.ones(ncells, dtype=np.int32)},
        )
        self.assertEqual(img.GetDimensions(), (3, 4, 5))
        self.assertAlmostEqual(img.GetSpacing()[0], 0.1)
        self.assertAlmostEqual(img.GetOrigin()[1], 2.0)
        self.assertEqual(img.point_data.keys(), ("temperature",))
        self.assertEqual(img.cell_data.keys(), ("material",))
        self.assertEqual(
            img.point_data["temperature"].GetNumberOfTuples(), npts
        )

    def test_image_data_constructor_multiple_arrays(self):
        """Pass multiple arrays per attribute in the constructor."""
        npts = 2 * 2 * 2
        img = vtkImageData(
            dimensions=(2, 2, 2),
            point_data={
                "scalars": np.zeros(npts, dtype=np.float32),
                "vectors": np.ones((npts, 3), dtype=np.float64),
            },
        )
        self.assertEqual(
            set(img.point_data.keys()), {"scalars", "vectors"}
        )
        self.assertEqual(
            img.point_data["vectors"].GetNumberOfComponents(), 3
        )


class TestEmptyDict(unittest.TestCase):
    """Setting an empty dict should clear all arrays."""

    def test_empty_dict_clears_point_data(self):
        img = vtkImageData()
        img.SetDimensions(2, 2, 2)
        npts = img.GetNumberOfPoints()
        img.point_data = {"x": np.zeros(npts)}
        self.assertEqual(len(img.point_data), 1)

        img.point_data = {}
        self.assertEqual(len(img.point_data), 0)

    def test_empty_dict_clears_cell_data(self):
        img = vtkImageData()
        img.SetDimensions(2, 2, 2)
        ncells = img.GetNumberOfCells()
        img.cell_data = {"x": np.zeros(ncells)}
        self.assertEqual(len(img.cell_data), 1)

        img.cell_data = {}
        self.assertEqual(len(img.cell_data), 0)


@unittest.skipUnless(HAS_PANDAS, "pandas is not installed")
class TestToPandas(unittest.TestCase):
    """Test to_pandas() on field data."""

    def setUp(self):
        self.img = vtkImageData()
        self.img.SetDimensions(3, 4, 5)
        self.npts = self.img.GetNumberOfPoints()

    def test_scalar_array(self):
        """Single-component array becomes one column."""
        self.img.point_data = {"temp": np.arange(self.npts, dtype=np.float64)}
        df = self.img.point_data.to_pandas()
        self.assertIsInstance(df, pd.DataFrame)
        self.assertEqual(list(df.columns), ["temp"])
        self.assertEqual(len(df), self.npts)

    def test_vector_array_split(self):
        """Multi-component array is split into name_0, name_1, ..."""
        self.img.point_data = {"vel": np.random.rand(self.npts, 3)}
        df = self.img.point_data.to_pandas()
        self.assertEqual(list(df.columns), ["vel_0", "vel_1", "vel_2"])

    def test_mixed_arrays(self):
        """Scalar and vector arrays together."""
        self.img.point_data = {
            "temp": np.zeros(self.npts),
            "vel": np.ones((self.npts, 3)),
        }
        df = self.img.point_data.to_pandas()
        self.assertEqual(
            list(df.columns), ["temp", "vel_0", "vel_1", "vel_2"]
        )

    def test_empty(self):
        """Empty field data gives empty DataFrame."""
        df = self.img.point_data.to_pandas()
        self.assertEqual(len(df.columns), 0)

    def test_values_roundtrip(self):
        """Values survive conversion."""
        data = np.linspace(0, 1, self.npts)
        self.img.point_data = {"x": data}
        df = self.img.point_data.to_pandas()
        np.testing.assert_allclose(df["x"].values, data)


@unittest.skipUnless(HAS_PANDAS, "pandas is not installed")
class TestFromPandas(unittest.TestCase):
    """Test from_pandas() on field data."""

    def setUp(self):
        self.img = vtkImageData()
        self.img.SetDimensions(3, 4, 5)
        self.npts = self.img.GetNumberOfPoints()
        self.df = pd.DataFrame({
            "pressure": np.ones(self.npts, dtype=np.float64),
            "density": np.arange(self.npts, dtype=np.float32),
        })

    def test_columns_become_arrays(self):
        """Each DataFrame column becomes a VTK array."""
        self.img.point_data.from_pandas(self.df)
        self.assertEqual(
            set(self.img.point_data.keys()), {"pressure", "density"}
        )

    def test_replaces_existing(self):
        """from_pandas clears previous arrays."""
        self.img.point_data = {"old": np.zeros(self.npts)}
        self.img.point_data.from_pandas(self.df)
        self.assertNotIn("old", self.img.point_data.keys())

    def test_values_roundtrip(self):
        """Values survive conversion."""
        self.img.point_data.from_pandas(self.df)
        arr = self.img.point_data["pressure"]
        np.testing.assert_allclose(np.asarray(arr), 1.0)

    def test_on_cell_data(self):
        """from_pandas works on cell_data too."""
        ncells = self.img.GetNumberOfCells()
        df = pd.DataFrame({"mat": np.ones(ncells, dtype=np.int32)})
        self.img.cell_data.from_pandas(df)
        self.assertEqual(self.img.cell_data.keys(), ("mat",))


@unittest.skipUnless(HAS_XARRAY, "xarray is not installed")
class TestToXarray(unittest.TestCase):
    """Test to_xarray() on field data."""

    def setUp(self):
        self.img = vtkImageData()
        self.img.SetDimensions(3, 4, 5)
        self.npts = self.img.GetNumberOfPoints()

    def test_scalar_array(self):
        """Single-component array gets dimension ('index',)."""
        self.img.point_data = {"temp": np.arange(self.npts, dtype=np.float64)}
        ds = self.img.point_data.to_xarray()
        self.assertIsInstance(ds, xr.Dataset)
        self.assertIn("temp", ds.data_vars)
        self.assertEqual(ds["temp"].dims, ("index",))
        self.assertEqual(ds["temp"].size, self.npts)

    def test_vector_array(self):
        """Multi-component array gets dimensions ('index', 'component')."""
        self.img.point_data = {"vel": np.random.rand(self.npts, 3)}
        ds = self.img.point_data.to_xarray()
        self.assertEqual(ds["vel"].dims, ("index", "component"))
        self.assertEqual(ds["vel"].shape, (self.npts, 3))

    def test_mixed_arrays(self):
        """Scalar and vector arrays in one Dataset."""
        self.img.point_data = {
            "temp": np.zeros(self.npts),
            "vel": np.ones((self.npts, 3)),
        }
        ds = self.img.point_data.to_xarray()
        self.assertEqual(set(ds.data_vars), {"temp", "vel"})

    def test_empty(self):
        """Empty field data gives empty Dataset."""
        ds = self.img.point_data.to_xarray()
        self.assertEqual(len(ds.data_vars), 0)

    def test_values_roundtrip(self):
        """Values survive conversion."""
        data = np.linspace(0, 1, self.npts)
        self.img.point_data = {"x": data}
        ds = self.img.point_data.to_xarray()
        np.testing.assert_allclose(ds["x"].values, data)


@unittest.skipUnless(HAS_XARRAY, "xarray is not installed")
class TestFromXarray(unittest.TestCase):
    """Test from_xarray() on field data."""

    def setUp(self):
        self.img = vtkImageData()
        self.img.SetDimensions(3, 4, 5)
        self.npts = self.img.GetNumberOfPoints()
        self.ds = xr.Dataset({
            "field_a": ("index", np.arange(self.npts, dtype=np.float32)),
            "field_b": (("index", "component"), np.ones((self.npts, 2))),
        })

    def test_variables_become_arrays(self):
        """Each xarray variable becomes a VTK array."""
        self.img.point_data.from_xarray(self.ds)
        self.assertEqual(
            set(self.img.point_data.keys()), {"field_a", "field_b"}
        )

    def test_multicomponent_preserved(self):
        """2D xarray variable becomes multi-component VTK array."""
        self.img.point_data.from_xarray(self.ds)
        self.assertEqual(
            self.img.point_data["field_b"].GetNumberOfComponents(), 2
        )

    def test_replaces_existing(self):
        """from_xarray clears previous arrays."""
        self.img.point_data = {"old": np.zeros(self.npts)}
        self.img.point_data.from_xarray(self.ds)
        self.assertNotIn("old", self.img.point_data.keys())

    def test_values_roundtrip(self):
        """Values survive conversion."""
        self.img.point_data.from_xarray(self.ds)
        arr = self.img.point_data["field_a"]
        expected = np.arange(self.npts, dtype=np.float32)
        np.testing.assert_allclose(np.asarray(arr), expected)

    def test_on_cell_data(self):
        """from_xarray works on cell_data too."""
        ncells = self.img.GetNumberOfCells()
        ds = xr.Dataset({"mat": ("index", np.ones(ncells))})
        self.img.cell_data.from_xarray(ds)
        self.assertEqual(self.img.cell_data.keys(), ("mat",))


if __name__ == "__main__":
    vtkTesting.main([(TestImageDataProperties, "test"),
                     (TestPolyDataProperties, "test"),
                     (TestRectilinearGridProperties, "test"),
                     (TestConstructor, "test"),
                     (TestEmptyDict, "test"),
                     (TestToPandas, "test"),
                     (TestFromPandas, "test"),
                     (TestToXarray, "test"),
                     (TestFromXarray, "test")])
