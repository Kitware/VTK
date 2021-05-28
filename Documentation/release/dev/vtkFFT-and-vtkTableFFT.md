## Better support of the FFT in VTK

Using the FFT within **VTK** is now easier and more performant. There's also some new
advanced feature.

---

For an FFT usage with stl container :

  - The `vtkFFT` class now implements the FFT, its inverse as well as both their optimized
    version for real input. The class uses the numpy terminology for these function names.
    The implementation is done using the *kissfft* library.
  - It also has utilities function for dealing with complex numbers and getting useful
  informations such as the frequency array for a given length and sampling rate.
  - Finally `vtkFFT` has functions for generating 1D and 2D kernels. For now it is possible to
  construct Hanning, Bartlett, Sine, Blackman and Rectangular kernels.

---

The `vtkTableFFT` class now uses the `vtkFFT` class for its implementation which is faster than the
former implementation. It also offers more feature such as :

 - Creating a frequency column in the output table
 - Doing the FFT per block and then computing the average of all blocks to get the result
 - Applying a window function before computing the FFT
