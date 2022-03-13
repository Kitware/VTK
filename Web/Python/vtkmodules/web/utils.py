import base64
import numpy as np

from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter

# Numpy to JS TypedArray
to_js_type = {
    "int8": "Int8Array",
    "uint8": "Uint8Array",
    "int16": "Int16Array",
    "uint16": "Uint16Array",
    "int32": "Int32Array",
    "uint32": "Uint32Array",
    "int64": "Int32Array",
    "uint64": "Uint32Array",
    "float32": "Float32Array",
    "float64": "Float64Array",
}


def b64_encode_numpy(obj):
    # Convert 1D numpy arrays with numeric types to memoryviews with
    # datatype and shape metadata.
    if len(obj) == 0:
        return obj.tolist()

    dtype = obj.dtype
    if dtype.kind == "f":
        return np_encode(obj)
    elif dtype.kind == "b":
        return np_encode(obj, np.uint8)
    elif dtype.kind in ["u", "i"]:
        # Try to see if we can downsize the array
        max_value = np.amax(obj)
        min_value = np.amin(obj)
        signed = min_value < 0
        test_value = max(max_value, -min_value)
        if signed:
            if test_value < np.iinfo(np.int8):
                return np_encode(obj, np.int8)
            if test_value < np.iinfo(np.int16).max:
                return np_encode(obj, np.int16)
            if test_value < np.iinfo(np.int32).max:
                return np_encode(obj, np.int32)
        else:
            if test_value < np.iinfo(np.uint8).max:
                return np_encode(obj, np.uint8)
            if test_value < np.iinfo(np.uint16).max:
                return np_encode(obj, np.uint16)
            if test_value < np.iinfo(np.uint32).max:
                return np_encode(obj, np.uint32)

    # Convert all other numpy arrays to lists
    return obj.tolist()


def np_encode(array, np_type=None):
    if np_type:
        n_array = array.astype(np_type).ravel(order="C")
        return {
            "bvals": base64.b64encode(memoryview(n_array)).decode("utf-8"),
            "dtype": str(n_array.dtype),
            "shape": list(array.shape),
        }
    return {
        "bvals": base64.b64encode(memoryview(array.ravel(order="C"))).decode("utf-8"),
        "dtype": str(array.dtype),
        "shape": list(array.shape),
    }


def mesh_array(array):
    if array:
        return b64_encode_numpy(vtk_to_numpy(array.GetData()))


def data_array(data_array, location="PointData", name=None):
    if data_array:
        dataRange = data_array.GetRange(-1)
        nb_comp = data_array.GetNumberOfComponents()
        values = vtk_to_numpy(data_array)
        js_types = to_js_type[str(values.dtype)]
        return {
            "name": name if name else data_array.GetName(),
            "values": b64_encode_numpy(values),
            "numberOfComponents": nb_comp,
            "type": js_types,
            "location": location,
            "dataRange": dataRange,
        }


def field_data(field_data, names, location="PointData"):
    fields = []
    for name in names:
        array = field_data.GetArray(name)
        js_array = data_array(array, location, name)
        if js_array:
            fields.append(js_array)

    return fields


def mesh(dataset, field_to_keep=None, point_arrays=None, cell_arrays=None):
    """Expect any dataset and extract its surface into a dash_vtk.Mesh state property"""
    if dataset is None:
        return None

    # Make sure we have a polydata to export
    polydata = None
    if dataset.IsA("vtkPolyData"):
        polydata = dataset
    else:
        extractSkinFilter = vtkDataSetSurfaceFilter()
        extractSkinFilter.SetInputData(dataset)
        extractSkinFilter.Update()
        polydata = extractSkinFilter.GetOutput()

    if polydata.GetPoints() is None:
        return None

    # Extract mesh
    state = {"mesh": {}}

    points = mesh_array(polydata.GetPoints())
    if points:
        state["mesh"]["points"] = points

    verts = mesh_array(polydata.GetVerts())
    if verts:
        state["mesh"]["verts"] = verts

    lines = mesh_array(polydata.GetLines())
    if lines:
        state["mesh"]["lines"] = lines

    polys = mesh_array(polydata.GetPolys())
    if polys:
        state["mesh"]["polys"] = polys

    strips = mesh_array(polydata.GetStrips())
    if strips:
        state["mesh"]["strips"] = strips

    # Scalars
    if field_to_keep is not None:
        field = None
        p_array = polydata.GetPointData().GetArray(field_to_keep)
        c_array = polydata.GetCellData().GetArray(field_to_keep)

        if c_array:
            field = data_array(c_array, location="CellData", name=field_to_keep)

        if p_array:
            field = data_array(p_array, location="PointData", name=field_to_keep)

        if field:
            state.update({"field": field})

    # PointData Fields
    if point_arrays:
        point_data = field_data(polydata.GetPointData(), point_arrays, "PointData")
        if len(point_data):
            state.update({"pointArrays": point_data})

    # CellData Fields
    if cell_arrays:
        cell_data = field_data(polydata.GetCellData(), cell_arrays, "CellData")
        if len(cell_data):
            state.update({"cellArrays": cell_data})

    return state


def volume(dataset):
    """Expect a vtkImageData and extract its setting for the dash_vtk.Volume state"""
    if dataset is None or not dataset.IsA("vtkImageData"):
        return None

    state = {
        "image": {
            "dimensions": dataset.GetDimensions(),
            "spacing": dataset.GetSpacing(),
            "origin": dataset.GetOrigin(),
        },
    }

    # Capture image orientation if any
    if hasattr(dataset, "GetDirectionMatrix"):
        matrix = dataset.GetDirectionMatrix()
        js_mat = []
        for j in range(3):
            for i in range(3):
                js_mat.append(matrix.GetElement(i, j))

        state["image"]["direction"] = js_mat

    scalars = dataset.GetPointData().GetScalars()
    field = data_array(scalars, location="PointData")
    if field:
        state["field"] = field

    return state
