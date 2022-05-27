# Add probing filter for vtkHyperTreeGrids and make PlotOverLine support vtkHyperTreeGrids

VTK now has a `vtkHyperTreeGridProbeFilter` that can be used to probe HyperTreeGrids using `vtkDataSet`. This
 functionality relies heavily on the `vtkHyperTreeGridGeometricLocator` also developed here.
