## Fix for vtkColorTransferFunction Logarithmic Scale Evaluation

vtkColorTransferFunction now correctly evaluates out-of-bounds and negative
scalar values when mapping colors on a logarithmic scale (`VTK_SCALE_LOG10`),
properly returning the designated above/below range colors without generating
undefined (`NaN`) values.
