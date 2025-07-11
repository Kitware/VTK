# Deprecate in `vtkChartXY::GetMouseSelectionMode` and move it to `vtkChart` class

As `GetMouseSelectionMode` function is now used by both `vtkChartXY` and `vtkChartParallelCoordinate`, it has now been moved to `vtkChart` class. The static function with the same logic is now called `GetSelectionModeFromMouseModifiers`.
