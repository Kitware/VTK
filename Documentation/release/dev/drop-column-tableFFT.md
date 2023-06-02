## Drop instead of squeeze input columns in vtkTableFFT

vtkTableFFT no longer adds or squeeze certain arrays, like those starting with `vtk` or having several components,
when the input and the output have a different size.

Previously, this could happen in 2 cases:
- with option `UseAverageFFT` : these arrays were added as it is causing an error because of the difference of size
with other arrays in the output.
- with option `OneSidedSpectrum` : in this case we make a squeeze on these arrays, this is no longer the case because
logical mapping of values is hard once squeezing is done.
