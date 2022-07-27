## Complex number support in vtkTableFFT class

It is now possible to process the FFT of complex values using the vtkTableFFT filter.
While reading columns of the input table, vtkTableFFT will check the number of components :
if there is 1 components then all imaginary values will be set to 0, else if there is
2 components then the second component will be interpreted as the imaginary part. Arrays with
more than 2 components are discarded.
