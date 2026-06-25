## Dataset attribute property setters and pandas/xarray conversion

VTK dataset objects now support `point_data`, `cell_data`, and `field_data`
property setters that accept dictionaries mapping names to arrays. This enables
streamlined dataset construction:

```python
img = vtkImageData(
    dimensions=(3, 4, 5),
    point_data={"temperature": np.arange(60, dtype=np.float64)},
)
```

The `FieldDataBase` class also provides `to_pandas()`, `from_pandas()`,
`to_xarray()`, and `from_xarray()` methods for seamless conversion between
VTK field data and pandas DataFrames or xarray Datasets. Multi-component
arrays are split into separate columns in pandas (e.g., `velocity_0`,
`velocity_1`, `velocity_2`) and preserved as multi-dimensional variables
in xarray.
