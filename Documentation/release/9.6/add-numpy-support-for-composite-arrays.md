# Add numpy support for composite arrays

You can now use VTK composite arrays with numpy functionnality just like non composite arrays.
In terms of available features:
- All numpy related algorithms previously accessible via algs (alias for numpy_interface.algorithms) are now accessible directly with their numpy counterpart (algs.mean -> np.mean)
- Most of in place numpy functions are now supported (typically math functions like cos, arctan, ...)
- Slicing is now available to extract specific elements or sub-arrays
- The old algs API is now deprecated for functions that have native NumPy equivalents

Note that the algs intermediate layer is still useful to access some VTK specific features, such as MPI related algs.

### Developer notes:

NumPy supports extension of its core functionality to non-`ndarray` types using two key protocols which were overriden in the `VTKCompositeDataArray` class:
- `__array_ufunc__`\
This method is called whenever a NumPy universal function (ufunc) like np.sin or np.sqrt is applied. Universal functions are more or less the elementwise functions.
- `__array_function__`\
This method is used for non-ufunc NumPy functions, such as np.mean or np.expand_dims. It is used for functions that imply structural manipulation, and is thus more general than `__array_ufunc__`. We register supported functions explicitly and dispatch to custom implementations. The registration mechanism is handled by a simple dictionnary and a function used as decorator.
