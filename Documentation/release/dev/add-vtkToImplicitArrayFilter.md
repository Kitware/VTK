## vtkToImplicitArrayFilter: compress explicit memory arrays into a `vtkImplicitArray`s

You now have access to a new filter `vtkToImplicitArrayFilter` in a new `FiltersReduction` module that can transform explicit memory arrays into `vtkImplicitArray`s. The filter itself delegates the "compression" method to a `vtkToImplicitStrategy` object using a strategy pattern.

The following strategies (in order of complexity) have been implemented so far:
- `vtkToConstantArrayStrategy`: transform an explicit array that has the same value everywhere into a `vtkConstantArray`
- `vtkToAffineArrayStrategy`: transform an explicit array that follows an affine dependence on its indexes into a `vtkAffineArray`
- `vtkToImplicitTypeErasureStrategy`: transform an explicit integral array (with more range in its value type than necessary for describing it) into a reduced memory explicit integral array wrapped in an implicit array.
- `vtkToImplicitRamerDouglasPeuckerStrategy`: transform an explicit memory array into a `vtkCompositeArray` with constant (`vtkConstantArray`) and affine (`vtkAffineArray`) parts following a Ramer-Douglas-Peucker algorithm.
