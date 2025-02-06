import sys
from vtkmodules.vtkIONetCDF import vtkNetCDFCFReader


# make sure the xarray is kept in memory
ds_xrs = []
def get_reader(file_name):
    print(sys.argv[2])
    if sys.argv[2] == "xarray":
        global ds_xrs
        print(f"Using XArray on: {file_name}")
        try:
            import xarray as xr
            from vtkmodules.util import xarray_support
        except ImportError:
            print("This test requires xarray!")
            from vtkmodules.test import Testing
            Testing.skip()
        ds_xr = xr.open_dataset(file_name)
        ds_xrs.append(ds_xr)
        reader = ds_xr.vtk.create_reader()
    else:
        print(f"Using NetCDFCFReader on: {file_name}")
        reader = vtkNetCDFCFReader()
        reader.SetFileName(file_name)
    return reader
