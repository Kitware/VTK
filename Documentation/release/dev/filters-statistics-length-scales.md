## Length-scale distribution

VTK now includes a fast (threaded, sub-linear) filter
for estimating the range of geometric length-scales
present in a vtkDataSet. This filter makes use of
the new vtkReservoirSampler implementation.

See `Filters/Statistics/vtkLengthDistribution.h`
and its test for details.
