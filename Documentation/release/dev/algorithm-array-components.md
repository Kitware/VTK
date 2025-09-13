## Querying and resetting arrays to process

To support filters that allow users to configure an arbitrary number of
arrays to process, `vtkAlgorithm` now provides two new methods:

+ `GetNumberOfInputArraySpecifications()` returns the number of arrays
  the algorithm is currently configured to process; and
+ `ResetInputArraySpecifications()` removes all array specifications
  so the filter behaves as if `SetInputArrayToProcess()` has never been
  called. This also marks the filter as modified if it had an effect.

## Specify array components to process

Your algorithm or pipeline can now specify not just an array to process,
but also a component of that array, using new overloaded signatures of the
`vtkAlgorithm::SetInputArrayToProcess()` method. This also includes support
for L₁, L₂, or L∞ norms of tuples rather than choosing a single tuple to
be processed. The different norms are computed like so:

+ The L₁ norm is the sum of the absolute values of each component in a tuple.
  For a tuple (3, -4, 12), the L₁ norm is |3| + |-4| + |12| = 19.
+ The L₂ norm is the square root of the sum of the squares of each component
  in the input tuple. For a tuple (3, -4, 12), the L₂ norm
  is sqrt(9 + 16 + 144) = 13.
+ The L∞ norm is the maximum of the absolute value of each component.
  For a tuple (3, 4, -12), the L∞ norm is max(|3|,|4|,|-12|) = 12.

Similarly, algorithms can now ask their parent `vtkAlgorithm` class to pass
them a single-component array holding the component/norm requested above
rather than extracting the component of the array specified by the pipeline.

### Asking a filter to process input components

Let's consider 3 examples to illustrate what's changed and what hasn't.
We'll assume your data has a vector-valued cell-data array named `velocity` with 3 components
and you want to pass it to a `filter` that uses a single component of the array.

1. Call `filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELL, "velocity");`
   to have `filter` treat each tuple as it has in the past (where each tuple has 3 components).
   This uses pre-existing methods and works just as it has in the past.
2. Call `filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELL, "velocity", 1);`
   to have `filter` use only the _y_ component of the velocity vector.
3. Call `filter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELL, "velocity", vtkArrayComponents::L2Norm);`
   to have `filter` process the L₂-norm of the each velocity tuple.

As you can see, `SetInputArrayToProcess` now takes an optional value at the end of its arguments
to indicate either a non-negative component number or a special enumerant from the `vtkArrayComponents`
enum to indicate a norm should be computed.

### Processing array components inside your algorithm

There is also a new method of `vtkAlgorithm` called `GetInputArrayComponent()` that you can use
to determine whether the pipeline is requesting a specific component or not.
If the array is intended to be taken as-is (i.e., use all components of the tuple), then the
value returned will be `vtkArrayComponents::AllComponents`.
Otherwise, the returned integer will be either non-negative (indicating a specific component)
or a value from the `vtkArrayComponents` enum indicating a norm should be computed.

To make using specific components of arrays easier, `vtkAlgorithm` now also provides methods named
`vtkAlgorithm::GetInputArray()` that will *either* return the original array or – if a
component/norm is requested – create a new array instance that holds the configured
component (or computed norm). It calls the new `vtk::ComponentOrNormAsArray()` in its implementation.

String- and variant-arrays support production of new single-component arrays of the
same type as the input for components (but obviously do not support computing norms).
To implement this behavior, the `CopyComponent()` method of `vtkDataArray` has been moved
to `vtkAbstractArray` and its signature modified to accept abstract
arrays as inputs. The original signature on `vtkDataArray` has been kept as an overload to avoid
the overhead of safe-downcasting to a data-array.

Because a smart-pointer to the array is returned, you can use it inside `RequestData()` without
worrying about deleting it before returning. You can also override the requested component it
fetches if your algorithm needs access to other tuple data.

There is also a templated method `GetInputArrayAs<ArrayType>()` that will return a smart-pointer
of the templated parameter (usually `vtkDataArray`) so you can call methods as needed.

To match the 3 examples above, you can simply call

```cpp
  vtkDataObject* input;
  int association;
  vtkSmartPointer<vtkDataArray> arrayToProcess = filter->GetInputArray(0, input, association);
```

and you'll be passed – depending on which of the 3 respective cases above you consider:

1. the original array with 3 components (_x_, _y_, _z_) as the user indicated `velocity`
   should be processed as a vector.
2. a new array of the same type with 1 component but the same number of tuples as the `velocity` array;
   each tuple will hold only the _y_ component of `velocity`).
3. a new double-valued array with 1 component but the same number of tuples as the `velocity` array;
   each tuple will hold the L₂ norm (= sqrt(_x² + y² + z²_)) of each tuple in `velocity`.

When L₁ or L₂ norms are requested, the array returned to you will have `double`-precision values.
All other norm- and component-selections will return arrays whose type matches the input array type.
Thus, if `velocity` was a `vtkCharArray` and an L∞ norm was requested, you will be passed a
`char`-valued array back.
This was done since the L₁ and L₂ norms may run into overflow and/or precision issues if the
storage type of component values is used to hold norm values.

Futhermore, if your algorithm requires a single-component array, you can pass an additional
`requestedComponent` parameter to `GetInputArray()` specifying a default component (usually 0)
or norm so that if the user configures a multi-component array you will always extract a
meaningful default component. For example, consider this snippet:

```cpp
  vtkAlgorithm* filter;
  int compOrNorm;
  filter->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "a1", compOrNorm);
  vtkSmartPointer<vtkDataArray> arrayToProcess =
    filter->GetInputArray(0, input, association, vtkArrayComponents::L2Norm);
```

If `compOrNorm` is

+ a valid component index or one of the norm enumerants of `vtkArrayComponents`,
  then `arrayToProcess` will be the specified component or norm.
+ `vtkArrayComponents::AllComponents`, then `arrayToProcess` will be a single-component
  array that evaluates to the L₂-norm of each tuple of "a1".

This way, `arrayToProcess` will never be a multi-component array, regardless of
whether the "a1" point-data array on the input dataset is.

## Modified vtkDataArray::CopyComponents signature

As mentioned above, the `CopyComponent()` method of `vtkDataArray` has been moved to
`vtkAbstractArray` and its signature modified to accept abstract arrays as inputs.
This means you may copy components of variant- and string-valued arrays (into other
variant- and string-arrays; no type conversion is attempted).

Also, it returns a boolean indicating whether component-copying was successful (true)
or not (false). The method will still print error messages in the case of failure.
The original signature on `vtkDataArray` has been kept as an overload to avoid the
overhead of safe-downcasting to a data-array.
