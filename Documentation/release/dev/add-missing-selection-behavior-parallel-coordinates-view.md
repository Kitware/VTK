## Add missing selection mode behavior in parallel coordinates chart

The parallel coordinates chart supports new keyboard shortcuts for different selection modes:
- Press `Ctrl` while selecting to add to the current selection.
- Press `Shift` while selecting to substract from the current selection.
- Press `Ctrl+Shift` while selecting to toggle compared to the current selection.

As `GetMouseSelectionMode` function is now used by both `vtkChartXY` and `vtkChartParallelCoordinates`, it has now been moved to the parent class `vtkChart`. The static function with the same logic is now called `GetSelectionModeFromMouseModifiers`. Moreover, `vtkChartXY::GetMouseSelectionMode` is now deprecated.
