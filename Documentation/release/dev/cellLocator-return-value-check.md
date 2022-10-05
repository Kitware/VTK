# Add checking of return values to vtkCellLocator

The FindClosestPointWithinRadius function calls EvaluatePosition several times but does not check its return value and so might use non-sensical data. This return value is now checked for improved stability.
