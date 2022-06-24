## Uniformize vtkPlot API

Uniformize the `vtkPlot` API for color setters/getter, in order to fit the API of `vtkPen` and `vtkBrush`. \
Methods using floating values as parameters (e.g. `vtkPlot::SetColor(double r, double g, double b)`) are now suffixed with `F`
to avoid confusion with equivalent functions using unigned chars. The former ones are marked as deprecated in VTK 9.3.
