## vtkCharParallelCoordinates: Improvements

`vtkChartParallelCoordinates` now has a chart legend, that is always positioned at the top right corner of the chart.
The legend can be enabled or disabled using the `SetShowLegend` method. The legend can be inlined (default) or not using
the `vtkChartLegend` API. Additionally, the `vtkPlotParallelCoordinates` now has the option to set a preconfigured color
array by using `SetColorModeToDefault`.
