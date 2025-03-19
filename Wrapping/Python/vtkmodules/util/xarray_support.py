import cftime
import logging
import numpy as np
from os.path import basename, splitext, exists
import xarray as xr
from vtkmodules.vtkCommonCore import (
    vtkVariant,
)
from vtkmodules.vtkCommonDataModel import (
    vtkDataObject
)
from vtkmodules.vtkCommonExecutionModel import (
    vtkAlgorithm,
    vtkStreamingDemandDrivenPipeline
)
from vtkmodules.vtkIONetCDF import vtkNetCDFCFReader, vtkXArrayAccessor
from vtkmodules.util import numpy_support
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

@xr.register_dataset_accessor("vtk")
class VtkAccessor:
    def __init__(self, dsxr):
        self._dsxr = dsxr

    def create_reader(self):
        '''
        Returns a vtkXArrayCFReader that reads data from the XArray
        (using zero-copy when possible). At the moment, data is copied
        for coordinates (because they are converted to double in the reader)
        and for certain data that is subset either in XArray or in VTK.
        Lazy loading in XArray is respected, that is data is accessed only when
        it is needed.
        Time is passed to VTK either as an int64 for datetime64 or timedelta64,
        or as a double (using cftime.toordinal) for cftime.
        '''
        reader = vtkXArrayCFReader()
        reader.SetXArray(self._dsxr)
        return reader


class vtkXArrayCFReader(VTKPythonAlgorithmBase):
    '''Reads data from a file using the XArray readers and then connects
    the XArray data to the vtkNetCDFCFREader (using zero-copy when
    possible). At the moment, data is copied for coordinates (because
    they are converted to double in the reader) and for certain data
    that is subset either in XArray or in VTK.  Lazy loading in XArray
    is respected, that is data is accessed only when it is needed.
    Time is passed to VTK either as an int64 for datetime64 or
    timedelta64, or as a double (using cftime.toordinal) for cftime.
    '''

    _FORWARD_GET = {
        "GetAccessor",
        "GetAllDimensions",

        "GetNumberOfVariableArrays",
        "GetAllVariableArrayNames",
        "GetVariableArrayName",
        "GetVariableArrayStatus",

        "GetTimeDimensionName",
        "GetLatitudeDimensionName",
        "GetLongitudeDimensionName",
        "GetVerticalDimensionName",

        "GetOutput",
        "GetOutputType",
        "GetSphericalCoordinates",

        "GetReplaceFillValueWithNan",

        "GetVariableDimensions",
        "GetVerticalBias",
        "GetVerticalScale",
        "PrintSelf",
    }
    _FORWARD_SET = {
        "SetDimensions",

        "SetTimeDimensionName",
        "SetLatitudeDimensionName",
        "SetLongitudeDimensionName",
        "SetVerticalDimensionName",


        "SetSphericalCoordinates",
        "SphericalCoordinatesOn",
        "SphericalCoordinatesOff",

        "SetReplaceFillValueWithNan",
        "ReplaceFillValueWithNanOn",
        "ReplaceFillValueWithNanOff",

        "SetOutputType",
        "SetOutputTypeToAutomatic",
        "SetOutputTypeToImage",
        "SetOutputTypeToRectilinear",
        "SetOutputTypeToStructured",
        "SetOutputTypeToUnstructured",

        "SetVariableArrayStatus",
        "SetVerticalBias",
        "SetVerticalScale",
        "UpdateMetaData",
    }

    def __init__(self):
        VTKPythonAlgorithmBase.__init__(
            self, nInputPorts=0, nOutputPorts=1, outputType="vtkDataObject"
        )
        self._log = logging.getLogger("vtkXArrayCFReader")
        self._filename = None
        self._timesteps = None
        self._timeindex = None
        self._node = None
        self._dsxr = None
        self._reader = vtkNetCDFCFReader()
        self._ndarray_cftime_toordinal = np.frompyfunc(vtkXArrayCFReader._cftime_toordinal, 1, 1)
        # reference to contiguous arrays so that they are not dealocated
        self._arrays = {}


    def __getattr__(self, name):
        in_set = name in self._FORWARD_SET
        in_get = name in self._FORWARD_GET
        if in_set or in_get:
            if in_set:
                self.Modified()
            return getattr(self._reader, name)
        else:
            raise AttributeError()

    def SetFileName(self, name):
        """Specify filename for the file to read."""
        if self._filename != name:
            self._filename = name
            self.Modified()

    def GetFileName(self):
        return self._filename

    def CanReadFile(self, filepath):
        ext = splitext(filepath)[1]
        filename = basename(filepath)
        correct_name = False
        if ext == '.nc' or ext == '.grib' or ext == '.h5':
            correct_name = True
        else:
            if ext == '' and filename == '.zgroup':
                correct_name = True
        if correct_name and exists(filepath):
            return 1
        else:
            return 0


    def SetNode(self, node):
        if self._node != node:
            self._node = node
            self.Modified()

    def GetNode(self):
        return self._node

    def SetXArray(self, dsxr):
        self._dsxr = dsxr
        self._update_accessor()
        self.Modified()

    def GetXArray(self):
        return self._dsxr

    def RequestDataObject(self, request, inInfo, outInfo):
        self._log.debug(f"DataObject ======================================================================")
        if not self._dsxr:
            if self._node:
                tree = xr.open_datatree(self._filename)
                self._dsxr = tree[self._node].to_dataset()
            else:
                self._dsxr = xr.open_dataset(self._filename, decode_timedelta=True)
            self._update_accessor()
        self._reader.UpdateDataObject()
        roi = self._reader.GetOutputInformation(0)
        if roi.Has(vtkDataObject.DATA_OBJECT()):
            rdata = roi.Get(vtkDataObject.DATA_OBJECT())
        else:
            self._log.error("vtkNetCDFCFReader did not create the dataset")
            rdata = None
        oi = outInfo.GetInformationObject(0)
        oi.Set(vtkDataObject.DATA_OBJECT(), rdata)
        return 1

    def RequestInformation(self, request, inInfo, outInfo):
        self._log.debug(f"Information ======================================================================")
        oi = outInfo.GetInformationObject(0)
        self._reader.UpdateInformation()
        roi = self._reader.GetOutputInformation(0)
        if roi.Has(vtkStreamingDemandDrivenPipeline.TIME_STEPS()):
            self._timesteps = roi.Get(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
            oi.Set(vtkStreamingDemandDrivenPipeline.TIME_STEPS(), self._timesteps, len(self._timesteps))
            oi.Set(vtkStreamingDemandDrivenPipeline.TIME_RANGE(), [self._timesteps[0], self._timesteps[-1]], 2)
            self._timesteps = np.asarray(self._timesteps)
        if roi.Has(vtkStreamingDemandDrivenPipeline.WHOLE_EXTENT()):
            ext = roi.Get(vtkStreamingDemandDrivenPipeline.WHOLE_EXTENT())
            self._log.debug("Whole extent: {}".format(ext))
            oi.Set(vtkStreamingDemandDrivenPipeline.WHOLE_EXTENT(), ext, 6)
        if roi.Has(vtkAlgorithm.CAN_HANDLE_PIECE_REQUEST()):
            oi.Set(vtkAlgorithm.CAN_HANDLE_PIECE_REQUEST(), 1)
        if roi.Has(vtkAlgorithm.CAN_PRODUCE_SUB_EXTENT()):
            oi.Set(vtkAlgorithm.CAN_PRODUCE_SUB_EXTENT(), 1)
        return 1

    def RequestUpdateExtent(self, request, inInfo, outInfo):
        self._log.debug(f"UpdateExtent ======================================================================")
        oi = outInfo.GetInformationObject(0)
        if oi.Has(vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP()):
            utime = oi.Get(vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP())
            timeindex = (np.abs(self._timesteps - utime)).argmin()
            if timeindex != self._timeindex:
                self._log.debug(f"Time index = {timeindex}")
                self._timeindex = timeindex
                self.Modified()
        if oi.Has(vtkStreamingDemandDrivenPipeline.UPDATE_EXTENT()):
            ext = [0, 0, 0, 0, 0, 0]
            oi.Get(vtkStreamingDemandDrivenPipeline.UPDATE_EXTENT(), ext)
            self._log.debug("Update extent: {}".format(ext))
            roi = self._reader.GetOutputInformation(0)
            roi.Set(vtkStreamingDemandDrivenPipeline.UPDATE_EXTENT(), ext, 6)
            self._reader.PropagateUpdateExtent()
        return 1


    def RequestData(self, request, inInfo, outInfo):
        self._log.debug(f"Data ======================================================================")
        if self._timeindex:
            dsxr = self._dsxr.isel({self.GetTimeDimensionName() : self._timeindex})
        else:
            # no time, so no aditional selection is needed
            dsxr = self._dsxr
        accessor = self._reader.GetAccessor()
        self._set_data_vars(accessor, dsxr)
        self._reader.Update()
        # self._reader's data is already set for this's data so no ShallowCopy is needed
        return 1

    @staticmethod
    def _get_nc_type(numpy_array_type):
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
            np.str_: NC_STRING,
            np.bytes_: NC_CHAR,
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

    def _update_accessor(self):
        accessor, timename = self._get_accessor()
        self._reader.SetAccessor(accessor)
        if timename:
            self._reader.SetTimeDimensionName(timename)

    def _get_accessor(self):
        acclog = logging.getLogger("_get_accessor_")
        acclog.setLevel(logging.WARNING)
        accessor = vtkXArrayAccessor()
        time_name = None
        time_names = []
        # Set Dim and DimLen
        dimNameToIndex = {k: i for i, k in enumerate(self._dsxr.sizes.keys())}
        accessor.SetDim(list(self._dsxr.sizes.keys()))
        accessor.SetDimLen(list(self._dsxr.sizes.values()))

        # Set Var
        varList = list(self._dsxr.data_vars.keys()) + list(self._dsxr.coords.keys())
        varNameToIndex = {k: i for i, k in enumerate(varList)}
        is_coord = [0] * len(self._dsxr.data_vars)
        is_coord = is_coord + [1] * len(self._dsxr.coords)
        coords_bounds = self._get_coords_bounds()
        accessor.SetVar(varList, is_coord)
        for i, v in enumerate(varList):
            # data_vars are set after array selection and time selection to
            # take advantage of xarray lazy loading
            # https://docs.xarray.dev/en/latest/internals/internal-design.html
            if is_coord[i] or v in coords_bounds:
                # if there is subsetting in xarray, self._dsxr[v].values is
                # not contiguous. If the array is not contiguous, a contiguous
                # copy is created otherwise the contiguous array is simply returned
                v_data = np.ascontiguousarray(self._dsxr[v].values)
                if (
                    v_data.dtype.type == np.datetime64
                    or v_data.dtype.type == np.timedelta64
                ):
                    un = np.datetime_data(v_data.dtype)
                    # unit = ns and 1 base unit
                    if un[0] == "ns" and un[1] == 1:
                        time_names.append(v)
                if v_data.dtype.char == "O":
                    # object array, assume cftime
                    # copy cftime array to a doubles array
                    self._arrays[v] = self._ndarray_cftime_toordinal(v_data).astype(np.float64)
                    time_names.append(v)
                    v_data = self._arrays[v]
                else:
                    self._arrays[v] = v_data
                acclog.debug(f"{v=} {v_data.shape=} {v_data.dtype} {self._dsxr[v].dims=}")
                acclog.debug(f"address:{hex(v_data.ctypes.data)}")
                accessor.SetVarValue(i, v_data)
                accessor.SetVarType(i, vtkXArrayCFReader._get_nc_type(v_data.dtype))
            else:
                accessor.SetVarType(i, vtkXArrayCFReader._get_nc_type(self._dsxr[v].variable.dtype))
            accessor.SetVarDims(i, [dimNameToIndex[name] for name in self._dsxr[v].dims])
            accessor.SetVarCoords(
                i, [varNameToIndex[name] for name in self._dsxr[v].coords]
            )

            acclog.debug("Attributes:")
            for item in self._dsxr[v].attrs.items():
                acclog.debug(
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

    def _set_data_vars(self, accessor, dsxr):
        # data_vars are listed first in the list of data_vars,coords so we don't
        # need to add coords to the list, and still get the corect indexes
        varList = list(dsxr.data_vars.keys())
        for i, v in enumerate(varList):
            if self._reader.GetVariableArrayStatus(v):
                v_data = np.ascontiguousarray(dsxr[v].values)
                accessor.SetVarValue(i, v_data)
                self._arrays[v] = v_data

    def _get_coords_bounds(self):
        '''
        Special data_vars associated coords
        '''
        b=set()
        for coord in list(self._dsxr.coords):
            bounds_attr = 'bounds'
            if bounds_attr in self._dsxr[coord].attrs:
                b.add(self._dsxr[coord].attrs[bounds_attr])
        return b


    @staticmethod
    def _cftime_toordinal(o):
        return o.toordinal(fractional=True)
