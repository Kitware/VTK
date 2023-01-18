## Improve vtkTupleInterpolator speed

Improve the interpolation computation in `vtkPiecewiseFunction`:
- we can decide to scan the data with Interpolation search instead of Binary search which will works better for a sorted and uniformly distributed array.
- `vtkPiecewiseFunction` will determine by itself which search method use.

Improve the interpolator initialization of `vtkTupleInterpolator` and `vtkSpline` when there is a lot of data:
- with `vtkSpline::FillFromDataPointer` and `vtkTupleInterpolator::FillFromData` we can fill the interpolator with all the data in one time and then, only compute one sort.
