# Add probing filter for vtkHyperTreeGrids and make PlotOverLine support vtkHyperTreeGrids

VTK now has a `vtkHyperTreeGridPProbeFilter` that can be used to probe HyperTreeGrids using `vtkDataSet`. This
 functionality relies heavily on the `vtkHyperTreeGridGeometricLocator` also developed here.

The new `vtkHyperTreeGridPProbeFilter` has also been integrated into the `vtkProbeLineFilter`.
