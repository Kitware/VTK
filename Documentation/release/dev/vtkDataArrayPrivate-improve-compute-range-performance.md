## vtkDataArrayPrivate: Improve DoCompute(Scalar/Vector)Range Performance

`vtkDataArrayPrivate::DoComputeScalarRange` and `vtkDataArrayPrivate::DoComputeVectorRange` have been optimized to
enhance performance by about 2x based on preliminary evaluation using the following strategies:

1. For arrays with number of components in [1-9], `vtkDataArrayPrivate::DoComputeScalarRange`'s performance is optimized
   by exploring loop unrolling and minimizing branching.
2. `vtkMathUtilities::UpdateRange` is inlined to reduce function call overhead.
3. `vtkMathUtilities::UpdateRangeFinite` is introduced to efficiently handle finite range updates.
4. For arrays with number of components in [1-9], `vtkDataArrayPrivate::DoComputeVectorRange`s performance is optimized
   by exploring loop unrolling and minimizing branching.

When calling FiniteRange functions, given that the probability of encountering infinity values is low in most cases,
the performance is improved by first computing the range without checking for infinity values, and only if the result
contains infinity values, the range is recomputed with infinity checks.
