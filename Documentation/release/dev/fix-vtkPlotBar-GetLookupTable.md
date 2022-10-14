## Fixes for vtkPlotBar.GetLookupTable

Fixes a bug where calling vtkPlotBar.GetLookupTable caused a segmentation
fault in the case where no data had been plotted yet.
