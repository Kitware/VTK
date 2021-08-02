## nest-chart-matrices

`vtkChartMatrix` now supports nested instances of other `vtkChartMatrix` instances.
The API is exactly similar to creating a new `vtkChart` in a chart matrix.

The `vtkChartMatrix::Paint` and `vtkChartMatrix::GetChartIndex` methods have
been heavily refactored to use a public layout iterator API.

The layout iterator API not only improves readability but also reduced the
code duplication, cognitive complexity within `Paint` and `GetChartIndex`.

These public iterator methods can be used to override the `Paint` method without
worrying about dealing with scene offsets, gutters, borders etc.
