# Fix vtkPlotBar::GetBounds logic when log scale is enabled

The `vtkPlotBar::GetBounds(double*, bool unscaled)` now correctly returns
unscaled bounds when `unscaled` is true and scaled bounds when `unscaled` is false.
