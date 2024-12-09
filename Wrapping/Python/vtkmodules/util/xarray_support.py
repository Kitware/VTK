import cftime
import inspect
import logging
import numpy as np
import xarray as xr
from vtkmodules.vtkCommonCore import (
    vtkVariant,
)
from vtkmodules.vtkIONetCDF import vtkNetCDFCFReader, vtkXArrayAccessor
from vtkmodules.util import numpy_support


def cftime_toordinal(o):
    return o.toordinal(fractional=True)


ndarray_cftime_toordinal = np.frompyfunc(cftime_toordinal, 1, 1)


def get_nc_type(numpy_array_type):
    """Returns a nc_type given a numpy array."""
    NC_BYTE = 1  # 1 byte integer
    NC_CHAR = 2  # iso/ascii character
    NC_SHORT = 3  # 2 byte integer
    NC_INT = 4  # 4 byte integer
    NC_LONG = NC_INT
    NC_FLOAT = 5
    NC_DOUBLE = 6
    NC_UBYTE = 7
    NC_USHORT = 8
    NC_UINT = 9
    NC_INT64 = 10  # 8 bypte integer
    NC_UINT64 = 11
    NC_STRING = 12
    _np_nc = {
        np.uint8: NC_UBYTE,
        np.uint16: NC_USHORT,
        np.uint32: NC_UINT,
        np.uint64: NC_UINT64,
        np.int8: NC_BYTE,
        np.int16: NC_SHORT,
        np.int32: NC_INT,
        np.int64: NC_INT64,
        np.float32: NC_FLOAT,
        np.float64: NC_DOUBLE,
        np.datetime64: NC_INT64,
        np.timedelta64: NC_INT64,
    }
    for key, nc_type in _np_nc.items():
        if (
            numpy_array_type == key
            or np.issubdtype(numpy_array_type, key)
            or numpy_array_type == np.dtype(key)
        ):
            return nc_type
    raise TypeError(
        "Could not find a suitable NetCDF type for %s" % (str(numpy_array_type))
    )


@xr.register_dataset_accessor("vtk")
class VtkAccessor:
    def __init__(self, xarray_obj):
        # logging.basicConfig(level=logging.DEBUG)
        self._xr = xarray_obj
        # reference to contiguous arrays so that they are not dealocated
        self._arrays = {}

    def reader(self):
        '''
        Returns a vtkNetCDFCFReader that reads data from the XArray
        (using zero-copy when possible). At the moment data is copied
        for coordinates (because they are converted to double in the reader)
        and for certain data that is subset either in XArray or in VTK.
        WARNING: the XArray has to be kept in memory while using the reader,
        otherwise you'll get a segfault.
        Time is passed to VTK either as an int64 for datetime64 or timedelta64,
        or as a double (using cftime.toordinal) for cftime.
        '''
        accessor, time_name = self.get_accessor()
        reader = vtkNetCDFCFReader(accessor=accessor)
        if time_name:
            reader.SetTimeDimensionName(time_name)
        return reader

    def get_accessor(self):
        accessor = vtkXArrayAccessor()
        time_name = None
        time_names = []
        # Set Dim and DimLen
        dims = {k: i for i, k in enumerate(self._xr.sizes.keys())}
        accessor.SetDim(list(self._xr.sizes.keys()))
        accessor.SetDimLen(list(self._xr.sizes.values()))
        # Set Var
        var = list(self._xr.data_vars.keys()) + list(self._xr.coords.keys())
        is_coord = [0] * len(self._xr.data_vars)
        is_coord = is_coord + [1] * len(self._xr.coords)
        accessor.SetVar(var, is_coord)
        for i, v in enumerate(var):
            # if there is subsetting in xarray, self._xr[v].values is
            # not contiguous. If the array is not contigous, a contigous
            # copy is created otherwise nothing is done.
            v_data = np.ascontiguousarray(self._xr[v].values)
            if (
                v_data.dtype.type == np.datetime64
                or v_data.dtype.type == np.timedelta64
            ):
                un = np.datetime_data(v_data.dtype)
                # unit = ns and 1 base unit in a spep
                if un[0] == "ns" and un[1] == 1 and v in self._xr.coords.keys():
                    time_names.append(v)
            if v_data.dtype.char == "O":
                # object array, assume cftime
                # copy cftime array to a doubles array
                self._arrays[v] = ndarray_cftime_toordinal(v_data).astype(np.float64)
                time_names.append(v)
                v_data = self._arrays[v]
            else:
                self._arrays[v] = v_data
            logging.debug(f"{v=} {v_data.shape=} {v_data.dtype} {self._xr[v].dims=}")
            logging.debug(f"address:{hex(v_data.ctypes.data)} {v_data=}")
            accessor.SetVarValue(i, v_data)
            accessor.SetVarType(i, get_nc_type(v_data.dtype))
            accessor.SetVarDimId(i, [dims[name] for name in self._xr[v].dims])
            logging.debug("Attributes:")
            for item in self._xr[v].attrs.items():
                logging.debug(
                    "name: {} value: {} type: {}".format(
                        item[0], item[1], type(item[1])
                    )
                )
                if np.issubdtype(type(item[1]), np.integer):
                    accessor.SetAtt(i, item[0], vtkVariant(int(item[1])))
                elif np.issubdtype(type(item[1]), np.floating):
                    accessor.SetAtt(i, item[0], vtkVariant(float(item[1])))
                elif isinstance(item[1], np.ndarray):
                    accessor.SetAtt(
                        i, item[0], vtkVariant(numpy_support.numpy_to_vtk(item[1]))
                    )
                else:
                    accessor.SetAtt(i, item[0], vtkVariant(item[1]))
        if len(time_names) >= 1:
            for name in time_names:
                if accessor.IsCOARDSCoordinate(name):
                    time_name = name
                    break
        return accessor, time_name
