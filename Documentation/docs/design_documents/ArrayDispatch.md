# vtkArrayDispatch and Related Tools

## Background

VTK datasets store most of their important information in subclasses of
`vtkAbstractArray`. Vertex locations (`vtkPoints::Data`), cell topology
(`vtkCellArray::Offsets/Connectivity`), and numeric point, cell, and generic attributes
(`vtkFieldData::Data`) are the dataset features accessed most frequently by VTK
algorithms, and these all rely on the `vtkAbstractArray` API.

## Terminology

This page uses the following terms:

An __ArrayType__ is a concrete subclass of `vtkAbstractArray`. For instance:

1. `vtkAOSDataArrayTemplate<float>` or `vtkFloatArray`,
2. `vtkSOADataArrayTemplate<vtkTypeInt64>` or `vtkSOATypeInt64Array`,
3. `vtkConstantArray<vtkTypeInt8>` or `vtkConstantTypeInt8Array`,
4. `vtkStringArray`,
5. `vtkBitArray`.

An __ArrayTypeTag__ is a constexpr integer value that represents an _class_ of arrays
which can be either templated or non-templated. For instance:

1. `vtkAOSDataArrayTemplate<float>` has an ArrayTypeTag of
   `std::integral_constant<int, vtkArrayTypes::VTK_AOS_DATA_ARRAY>`,
2. `vtkSOADataArrayTemplate<vtkTypeInt64>` has an ArrayTypeTag of
   `std::integral_constant<int, vtkArrayTypes::VTK_SOA_DATA_ARRAY>`,
3. `vtkConstantArray<vtkTypeInt8>` has an ArrayTypeTag of
   `std::integral_constant<int, vtkArrayTypes::VTK_CONSTANT_ARRAY>`,
4. `vtkStringArray` has an ArrayTypeTag of
   `std::integral_constant<int, vtkArrayTypes::VTK_STRING_ARRAY>`,
5. `vtkBitArray` has an ArrayTypeTag of `std::integral_constant<int, vtkArrayTypes::VTK_BIT_ARRAY>`.

The integer values used in ArrayTypeTag can be found in `vtkType.h`, and queried using
`int vtkAbstractArray::GetArrayType()`.

A __DataTypeTag__ is constexpr integer value that represents a ValueType of an ArrayType. For instance:

1. `vtkAOSDataArrayTemplate<float>` has a DataTypeTag of `std::integral_constant<int, VTK_FLOAT>`,
2. `vtkSOADataArrayTemplate<vtkTypeInt64>` has a DataTypeTag of `std::integral_constant<int, VTK_TYPE_INT64>`,
3. `vtkConstantArray<vtkTypeInt8>` has a DataTypeTag of `std::integral_constant<int, VTK_TYPE_INT8>`,
4. `vtkStringArray` has a DataTypeTag of `std::integral_constant<int, VTK_STRING>`,
5. `vtkBitArray` has a DataTypeTag of `std::integral_constant<int, VTK_BIT>`.

The integer values used in DataTypeTag can be found in `vtkType.h`, and queried using
`int vtkAbstractArray::GetDataType()`.

A __ValueType__ is the element type of an ArrayType. For instance:

1. `vtkAOSDataArrayTemplate<float>` or `vtkFloatArray` have a ValueType of `float`,
2. `vtkSOADataArrayTemplate<vtkTypeInt64>` or `vtkSOATypeInt64Array` have a ValueType of `vtkTypeInt64`,
3. `vtkConstantArray<vtkTypeInt8>` or `vtkConstantTypeInt8Array` have a ValueType of `vtkTypeInt8`,
4. `vtkStringArray` has a ValueType of `vtkStdString`,
5. `vtkBitArray` has a ValueType of `unsigned char`, because we can't have bit.

And Array must specify all of the above, i.e. ValueType, DataTypeTag, and ArrayTypeTag,
along with an array implementation. This becomes important as `vtkAbstractArray` concrete
subclasses will begin to stray from the typical "array-of-structs" ordering that has been
exclusively used in the past.

A __dispatch__ is a runtime-resolution of a `vtkAbstractArray`'s ArrayType, and is
used to call a section of executable code that has been tailored for that
ArrayType. Dispatching has compile-time and run-time components. At compile-time,
the possible ArrayTypes to be used are determined, and a worker code template is
generated for each type, which is stored in an array so that it can be efficiently
accessed using `GetArrayType()` and `GetDataType()` with `O(1)` complexity. At run-time,
the correct ArrayType is determined, using `GetArrayType()` and `GetDataType()`, and
the proper worker instantiation is called.

__Template explosion__ refers to a sharp increase in the size of a compiled
binary that results from instantiating a template function or class on many
different types.

### vtkAbstractArray

The abstract array type hierarchy in VTK has a unique feature when compared to
typical C++ containers: a non-templated base class. All arrays inherit
`vtkAbstractArray`. Those containing numeric data inherit `vtkDataArray`, which
is a subclass of `vtkAbstractArray`, a common interface that sports a very
useful API. Without knowing the underlying ValueType stored in data array, an
algorithm or user may still work with any `vtkDataArray` in meaningful ways:
The array can be resized, reshaped, read, and rewritten easily using a generic
API that substitutes double-precision floating point numbers for the array's
actual ValueType. For instance, we can write a simple function that computes
the magnitudes for a set of vectors in one array and store the results in
another using nothing but the typeless `vtkDataArray` API:

```cpp
// 3 component magnitude calculation using the vtkDataArray API.
// Inefficient, but easy to write:
void calcMagnitude(vtkDataArray *vectors, vtkDataArray *magnitude)
{
  vtkIdType numVectors = vectors->GetNumberOfTuples();
  for (vtkIdType tupleIdx = 0; tupleIdx < numVectors; ++tupleIdx)
  {
    // What data types are magnitude and vectors using?
    // We don't care! These methods all use double.
    magnitude->SetComponent(tupleIdx, 0, std::sqrt(
      vectors->GetComponent(tupleIdx, 0) * vectors->GetComponent(tupleIdx, 0) +
      vectors->GetComponent(tupleIdx, 1) * vectors->GetComponent(tupleIdx, 1) +
      vectors->GetComponent(tupleIdx, 2) * vectors->GetComponent(tupleIdx, 2));
  }
}
```

### The Costs of Flexibility

However, this flexibility comes at a cost. Passing data through a generic API
has a number of issues:

#### Accuracy

Not all ValueTypes are fully expressible as a `double`. The truncation of
integers with > 52 bits of precision can be a particularly nasty issue.

#### Performance

__Virtual overhead__: The only way to implement such a system is to route the
`vtkDataArray` calls through a run-time resolution of ValueTypes. This is
implemented through the virtual override mechanism of C++, which adds a small
overhead to each API call.

__Missed optimization__: The virtual indirection described above also prevents
the compiler from being able to make assumptions about the layout of the data
in-memory. This information could be used to perform advanced optimizations,
such as vectorization.

So what can one do if they want fast, optimized, type-safe access to the data
stored in a `vtkDataArray` or `vtkAbstractArray`? What options are available?

### The Old Solution: vtkTemplateMacro

The `vtkTemplateMacro`, or its extension `vtkExtendedTemplateMacro`
(adds `VTK_STRING`) and `vtkExtraExtendedTemplateMacro` (adds `VTK_STRING`,
`VTK_VARIANT`) is described in this section. While it is no longer
considered a best practice to use this construct in new code, it is still
usable and likely to be encountered when reading the VTK source code. Newer
code should use the `vtkArrayDispatch` mechanism, which is detailed later. The
discussion of `vtkTemplateMacro` will help illustrate some of the practical
issues with array dispatching.

With a few minor exceptions that we won't consider here, prior to VTK 7.1 it
was safe to assume that all numeric `vtkDataArray` objects were also subclasses
of `vtkAOSDataArrayTemplate`, which at the time was called `vtkDataArrayTemplate`.
This template class provided the implementation of all documented numeric data
arrays such as `vtkDoubleArray`, `vtkIdTypeArray`, etc., and stores the tuples
in memory as a contiguous array-of-structs (AOS). For example, if we had an array
that stored 3-component tuples as floating point numbers, we could define a tuple as:

```cpp
struct Tuple { float x; float y; float z; };
```

An array-of-structs, or AOS, memory buffer containing this data could be
described as:

```cpp
Tuple ArrayOfStructsBuffer[NumTuples];
```

As a result, `ArrayOfStructsBuffer` will have the following memory layout:

```cpp
{ x1, y1, z1, x2, y2, z2, x3, y3, z3, ...}
```

That is, the components of each tuple are stored in adjacent memory locations,
one tuple after another. While this is not exactly how `vtkAOSDataArrayTemplate`
implemented its memory buffers, it accurately describes the resulting memory
layout.

Combine the AOS memory convention and `GetDataType()` with a horrific little
method on the data arrays named `GetVoidPointer()`, and a path to efficient,
type-safe access was available. `GetVoidPointer()` does what it says on the
tin: it returns the memory address for the array data's base location as a
`void*`. While this breaks encapsulation and sets off warning bells for the
more pedantic among us, the following technique was safe and efficient when
used correctly:

```cpp
// 3-component magnitude calculation using GetVoidPointer.
// Efficient and fast, but assumes AOS memory layout
template <typename ValueType>
void calcMagnitudeWorker(ValueType* vectors, ValueType* magnitude, vtkIdType numVectors)
{
  for (vtkIdType tupleIdx = 0; tupleIdx < numVectors; ++tupleIdx)
  {
    // We now have access to the raw memory buffers, and assuming
    // AOS memory layout, we know how to access them.
    magnitude[tupleIdx] = std::sqrt(
      vectors[3 * tupleIdx + 0] * vectors[3 * tupleIdx + 0] +
      vectors[3 * tupleIdx + 1] * vectors[3 * tupleIdx + 1] +
      vectors[3 * tupleIdx + 2] * vectors[3 * tupleIdx + 2]);
   }
}

void calcMagnitude(vtkDataArray* vectors, vtkDataArray* magnitude)
{
  assert("Arrays must have same datatype!" &&
         vtkDataTypesCompare(vectors->GetDataType(),
                             magnitude->GetDataType()));
  switch (vectors->GetDataType())
  {
    vtkTemplateMacro(calcMagnitudeWorker<VTK_TT*>(
      static_cast<VTK_TT*>(vectors->GetVoidPointer(0)),
      static_cast<VTK_TT*>(magnitude->GetVoidPointer(0)),
      vectors->GetNumberOfTuples());
  }
}
```

The `vtkTemplateMacro`, as you may have guessed, expands into a series of case
statements that determine an array's ValueType from the `int GetDataType()`
return value. The ValueType is then `typedef`'d to `VTK_TT`, and the macro's
argument is called for each numeric type returned from `GetDataType`. In this
case, the call to `calcMagnitudeWorker` is made by the macro, with `VTK_TT`
`typedef`'d to the array's ValueType.

This is the typical usage pattern for `vtkTemplateMacro`. The `calcMagnitude`
function calls a templated worker implementation that uses efficient, raw
memory access to a typesafe memory buffer so that the worker's code can be as
efficient as possible. But this assumes AOS memory ordering, and as we'll
mention, this assumption may no longer be valid as VTK moves further into the
field of in-situ analysis.

But first, you may have noticed that the above example using `vtkTemplateMacro`
has introduced a step backwards in terms of functionality. In the
`vtkDataArray` implementation, we didn't care if both arrays were the same
ValueType, but now we have to ensure this, since we cast both arrays' `void`
pointers to `VTK_TT`*. What if vectors is an array of integers, but we want to
calculate floating point magnitudes?

### vtkTemplateMacro with Multiple Arrays

The best solution prior to VTK 7.1 was to use two worker functions. The first
is templated on vector's ValueType, and the second is templated on both array
ValueTypes:

```cpp
// 3-component magnitude calculation using GetVoidPointer and a
// double-dispatch to resolve ValueTypes of both arrays.
// Efficient and fast, but assumes AOS memory layout, lots of boilerplate
// code, and the sensitivity to template explosion issues increases.
template <typename VectorType, typename MagnitudeType>
void calcMagnitudeWorker2(VectorType* vectors, MagnitudeType* magnitude,
                          vtkIdType numVectors)
{
  for (vtkIdType tupleIdx = 0; tupleIdx < numVectors; ++tupleIdx)
  {
    // We now have access to the raw memory buffers, and assuming
    // AOS memory layout, we know how to access them.
    // We now have access to the raw memory buffers, and assuming
    // AOS memory layout, we know how to access them.
    magnitude[tupleIdx] = std::sqrt(
      vectors[3 * tupleIdx + 0] * vectors[3 * tupleIdx + 0] +
      vectors[3 * tupleIdx + 1] * vectors[3 * tupleIdx + 1] +
      vectors[3 * tupleIdx + 2] * vectors[3 * tupleIdx + 2]);
  }
}

// Vector ValueType is known (VectorType), now use vtkTemplateMacro on
// magnitude:
template <typename VectorType>
void calcMagnitudeWorker1(VectorType* vectors, vtkDataArray* magnitude,
                          vtkIdType numVectors)
{
  switch (magnitude->GetDataType())
  {
    vtkTemplateMacro(calcMagnitudeWorker2(vectors,
      static_cast<VTK_TT*>(magnitude->GetVoidPointer(0)), numVectors);
  }
}

void calcMagnitude(vtkDataArray *vectors, vtkDataArray *magnitude)
{
  // Dispatch vectors first:
  switch (vectors->GetDataType())
  {
    vtkTemplateMacro(calcMagnitudeWorker1<VTK_TT*>(
      static_cast<VTK_TT*>(vectors->GetVoidPointer(0)),
      magnitude, vectors->GetNumberOfTuples());
  }
}
```

This works well, but it's a bit ugly and has the same issue as before regarding
memory layout. Double dispatches using this method will also see more problems
regarding binary size. The number of template instantiations that the compiler
needs to generate is determined by `I = T^D`, where `I` is the number of
template instantiations, `T` is the number of types considered, and `D` is the
number of dispatches. As of VTK 7.1, `vtkTemplateMacro` considers 14 data
types, so this double-dispatch will produce 14 instantiations of
`calcMagnitudeWorker1` and 196 instantiations of `calcMagnitudeWorker2`. If we
tried to resolve 3 `vtkDataArray`s into raw C arrays, 2744 instantiations of
the final worker function would be generated. As more arrays are considered,
the need for some form of restricted dispatch becomes very important to keep
this template explosion in check.

### Data Array Changes in newer VTK

Starting with VTK 7.1, the Array-Of-Structs (AOS) memory layout is no longer
the only `vtkDataArray` implementation provided by the library. The
Struct-Of-Arrays (SOA) memory layout is now available through the
`vtkSOADataArrayTemplate` class. The SOA layout assumes that the components of
an array are stored separately, as in:

```cpp
struct StructOfArraysBuffer
{
  float *x; // Pointer to array containing x components
  float *y; // Same for y
  float *z; // Same for z
};
```

The new SOA arrays were added to improve interoperability between VTK and
simulation packages for live visualization of in-situ results. Many simulations
use the SOA layout for their data, and natively supporting these arrays in VTK
will allow analysis of live data without the need to explicitly copy it into a
VTK data structure.

As a result of this change, a new mechanism is needed to efficiently access
array data. `vtkTemplateMacro` and `GetVoidPointer` are no longer an acceptable
solution -- implementing `GetVoidPointer` for SOA arrays requires creating a
deep copy of the data into a new AOS buffer, a waste of both processor time and
memory. Even if someone is willing to pay the cost of this copy, `GetVoidPointer`
could only be used to read data from the SOA array, not write to it, so it would
be of limited use.

So we need a replacement for `vtkTemplateMacro` that can abstract away things
like storage details while providing performance that is on-par with raw memory
buffer operations (when possible). And while we're at it, let's look at removing
the tedium of multi-array dispatch and reducing the problem of 'template explosion'.
The remainder of this page details such a system.

## Best Practices for vtkDataArray access

We'll describe a new set of tools that make managing template instantiations
for efficient array access both easy and extensible. As an overview, the
following new features will be discussed:

* `vtkGenericDataArray`: The new templated base interface for all numeric
  `vtkDataArray` subclasses.
* `vtkArrayDispatch`: Collection of code generation tools that allow concise
  and precise specification of restrictable dispatch for up to 3 `vtkAbstractArray`s
  simultaneously.
* `vtkArrayDownCast`: Access to specialized downcast implementations from code
  templates.
* `vtk::DataArrayValueRange<TupleSize = vtk::detail::DynamicTupleSize, ForceValueTypeForVtkDataArray = double>()`:
  Range-based for loop support for `vtkAbstractArray` subclasses that abstracts
  away storage details and allows efficient access of values.
* `vtk::DataArrayTupleRange<TupleSize = vtk::detail::DynamicTupleSize>()`:
  Range-based for loop support for `vtkAbstractArray` subclasses that abstracts
  away storage details and allows efficient access of values.
* `vtkDataArrayAccessor`: A helper class that provides a single API for accessing/inserting
   values/tuples in any `vtkDataArray` subclass. This class was the predecessor of `vtk::DataArray(Value/Tuple)Range`.
   It should only be used when inserting is required, and not for value/tuple access, because it's not as optimized.

These will be discussed more fully, but as a preview, here's our familiar
`calcMagnitude` example implemented using these new tools:

```cpp
// Modern implementation of calcMagnitude using new concepts in VTK:
// A worker functor. The calculation is implemented in the function template
// for operator().
struct CalcMagnitudeWorker
{
  // The worker accepts VTK array objects now, not raw memory buffers.
  template <typename VectorArray, typename MagnitudeArray>
  void operator()(VectorArray* vectorsArray, MagnitudeArray* magnitudeArray)
  {
    // These allow this single worker function to be used with both
    // the vtkDataArray 'double' API and the more efficient
    // vtkGenericDataArray APIs, depending on the template parameters:
    auto vectors = vtk::DataArrayTupleRange<3>(vectorsArray);
    auto magnitude = vtk::DataArrayValueRange<1>(magnitudeArray);

    vtkIdType numVectors = vectorsArray->GetNumberOfTuples();
    for (vtkIdType tupleIdx = 0; tupleIdx < numVectors; ++tupleIdx)
    {
      // These operataors[] compile to inlined optimizable raw memory accesses
      // for vtkGenericDataArray subclasses.
      magnitude[tupleIdx] = std::sqrt(
        vectors[tupleIdx][0] * vectors[tupleIdx][0] +
        vectors[tupleIdx][1] * vectors[tupleIdx][1] +
        vectors[tupleIdx][2] * vectors[tupleIdx][2]);
    }
  }
};

void calcMagnitude(vtkDataArray *vectors, vtkDataArray *magnitude)
{
  // Create our worker functor:
  CalcMagnitudeWorker worker;

  // Define our dispatcher. We'll let vectors have any ValueType, but only
  // consider float/double arrays for magnitudes. These combinations will
  // use a 'fast-path' implementation generated by the dispatcher:
  using Dispatcher = vtkArrayDispatch::Dispatch2ByValueType
    <
      vtkArrayDispatch::AllTypes, // ValueTypes allowed by first array
      vtkArrayDispatch::Reals     // ValueTypes allowed by second array
    >;

  // Execute the dispatcher:
  if (!Dispatcher::Execute(vectors, magnitude, worker))
  {
    // If Execute() fails, it means the dispatch failed due to an
    // unsupported array type. In this case, it's likely that the magnitude
    // array is using an integral type. This is an uncommon case, so we won't
    // generate a fast path for these, but instead call an instantiation of
    // CalcMagnitudeWorker::operator()<vtkDataArray, vtkDataArray>.
    // Through the use of vtk::DataArrayTupleRange, this falls back to using the
    // vtkDataArray double API:
    worker(vectors, magnitude);
  }
}
```

## vtkGenericDataArray

The `vtkGenericDataArray` class template drives the new `vtkDataArray` class
hierarchy. The ValueType is introduced here, both as a template parameter and a
class-scope `typedef`. This allows a typed API to be written that doesn't
require conversion to/from a common type (as `vtkDataArray` does with double).
It does not implement any storage details, however. Instead, it uses the CRTP
idiom to forward key method calls to a derived class without using a virtual
function call. By eliminating this indirection, `vtkGenericDataArray` defines
an interface that can be used to implement highly efficient code, because the
compiler is able to see past the method calls and optimize the underlying
memory accesses instead.

There are two main subclasses of `vtkGenericDataArray`: `vtkAOSDataArrayTemplate`
and `vtkSOADataArrayTemplate`. These implement array-of-structs and struct-of-arrays
storage, respectively. Recently, `vtkImplicitArray` was added, which is also a subclass
of `vtkGenericDataArray`, and is used to implement read-only arrays using a given backend.
We won't go into detail how this is done, but one could look into `vtkConstantArray`
and `vtkAffineArray` which are examples of `vtkImplicitArray` subclasses.

## vtkTypeList

Type lists are a metaprogramming construct used to generate a list of C++
types. They are used in VTK to implement restricted array dispatching. As we will
see, `vtkArrayDispatch` offers ways to reduce the number of generated template
instantiations by enforcing constraints on the arrays used to dispatch. For
instance, if one wanted to only generate templated worker implementations for
`vtkFloatArray` and `vtkIntArray`, a typelist is used to specify this:

```cpp
// Create a typelist of 2 types, vtkFloatArray and vtkIntArray:
using MyArrays = vtkTypeList::Create<vtkFloatArray, vtkIntArray>;

Worker someWorker = ...;
vtkDataArray *someArray = ...;

// Use vtkArrayDispatch to generate code paths for these arrays:
vtkArrayDispatch::DispatchByArray<MyArrays>(someArray, someWorker);
```

There's not much to know about type lists as a user, other than how to create
them. As seen above, there is a set of macros named `vtkTypeList::Create<...>`,
where X is the number of types in the created list, and the arguments are the
types to place in the list. In the example above, the new type list is
typically bound to a friendlier name using a local `typedef`, which is a common
practice.

The `vtkTypeList.h` header defines some additional type list operations that
may be useful, such as deleting and appending types, looking up indices, etc.
`vtkArrayDispatch::FilterArraysByValueType` may come in handy, too. But for
working with array dispatches, most users will only need to create new ones, or
use one of the following predefined `vtkTypeLists`:

* `vtkArrayDispatch::Reals`: All floating point ValueTypes.
* `vtkArrayDispatch::Integrals`: All integral ValueTypes.
* `vtkArrayDispatch::AllTypes`: Union of Reals and Integrals.
* `vtkArrayDispatch::AOSArrays`: Default list of AOS ArrayTypes to use in dispatches,
  which is filled if `CMake` option `VTK_DISPATCH_AOS_ARRAYS` is on.
* `vtkArrayDispatch::SOAArrays`: Default list of SOA ArrayTypes to use in dispatches,
  which is filled if `CMake` option `VTK_DISPATCH_SOA_ARRAYS` is on.
* `vtkArrayDispatch::Arrays`: Default list of read/write ArrayTypes to use in dispatches,
  which is a union of `AOSArrays` and `SOAArrays`.
* `vtkArrayDispatch::AffineArrays`: Default list of affine ArrayTypes to use in dispatches,
  which is filled if `CMake` option `VTK_DISPATCH_AFFINE_ARRAYS` is on.
* `vtkArrayDispatch::ConstantArrays`: Default list of constant ArrayTypes to use in dispatches,
  which is filled if `CMake` option `VTK_DISPATCH_CONSTANT_ARRAYS` is on.
* `vtkArrayDispatch::StridedArrays`: Default list of strided ArrayTypes to use in dispatches,
  which is filled if `CMake` option `VTK_DISPATCH_STRIDED_ARRAYS` is on.
* `vtkArrayDispatch::StructuredPointArrays`: Default list of structured point ArrayTypes to use in dispatches,
  which is filled if `CMake` option `VTK_DISPATCH_STRUCTURED_POINT_ARRAYS` is on.
* `vtkArrayDispatch::ReadOnlyArrays`: Default list of read-only ArrayTypes to use in dispatches,
  which is union of `AffineArrays`, `ConstantArrays`, `StridedArrays`, and `StructuredPointArrays`.
* `vtkArrayDispatch::AllArrays`: Union of `vtkArrayDispatch::Arrays` and `vtkArrayDispatch::ReadOnlyArrays`.

There are also some additional predefined `vtkTypeLists` in `vtkArrayDispatchDataSetArrayList.h`:

* `vtkArrayDispatch::AOSPointArrays`: Float and double AOS arrays that are commonly used for point coordinates.
* `vtkArrayDispatch::PointArrays`: Float and double AOS and SOA arrays that are commonly used for point coordinates.
* `vtkArrayDispatch::AllPointArrays`: Float and double AOS, SOA, StructuredPoint arrays that are commonly used for point
  coordinates.
* `vtkArrayDispatch::ConnectivityArrays`: `vtkTypeInt32` and `vtkTypeInt64` AOS arrays that are commonly used for cell
  connectivity.
* `vtkArrayDispatch::OffsetsArrays`: `vtkTypeInt32` and `vtkTypeInt64` AOS and Affine arrays that are commonly used for
  cell offsets.
* `vtkArrayDispatch::CellTypesArrays`: unsigned char AOS and Constant arrays that are commonly used for cell types.

`vtkArrayDispatch::Arrays` is a special typelist of ArrayTypes set application-wide
when VTK is built. This `vtkTypeList` of `vtkDataArray` subclasses is used for unrestricted
dispatches, and is the list that gets filtered when restricting a dispatch to specific ValueTypes.

Refining `vtkArrayDispatch::Arrays` or `vtkArrayDispatch::AllArrays` can be done using
the aforementioned `CMake` options `VTK_DISPATCH_*_ARRAYS`, and allows the user building
VTK to have some control over the dispatch process. If SOA arrays (or others) are never going
to be used, they can be removed from dispatch calls, reducing compile times and binary size.
On the other hand, a user applying in-situ techniques may want them available, because they'll
be used to import views of intermediate results.

## vtkArrayDownCast

In VTK, all subclasses of `vtkObject` (including the data arrays) support a
downcast method called `SafeDownCast`. It is used similarly to the C++
`dynamic_cast` -- given an object, try to cast it to a more derived type or
return `nullptr` if the object is not the requested type. Say we have a
`vtkDataArray` and want to test if it is actually a `vtkFloatArray`. We can do
this:

```cpp
void DoSomeAction(vtkDataArray *dataArray)
{
  vtkFloatArray *floatArray = vtkFloatArray::SafeDownCast(dataArray);
  if (floatArray)
  {
    // ... (do work with float array)
  }
}
```

This works, but it can pose a serious problem if `DoSomeAction` is called
repeatedly. `SafeDownCast` works by performing a series of virtual calls and
string comparisons to determine if an object falls into a particular class
hierarchy. These string comparisons add up and can actually dominate
computational resources if an algorithm implementation calls `SafeDownCast` in
a tight loop.

In such situations, it's ideal to restructure the algorithm so that the
downcast only happens once and the same result is used repeatedly, but
sometimes this is not possible. To lessen the cost of downcasting arrays, a
`FastDownCast` method exists for all concrete subclasses of `vtkAbstractArray`. This
replaces the string comparisons with a single virtual call and a few integer
comparisons and is far cheaper than the more general SafeDownCast. However, not
all array implementations support the `FastDownCast` method, such as a vtkImplicitArray
with a backend that is not publicly defined.

This creates a headache for templated code. Take the following example:

```cpp
template <typename ArrayType>
void DoSomeAction(vtkAbstractArray *array)
{
  ArrayType *myArray = ArrayType::SafeDownCast(array);
  if (myArray)
  {
    // ... (do work with myArray)
  }
}
```

We cannot use `FastDownCast` here since not all possible ArrayTypes support it.
But we really want that performance increase for the ones that do --
`SafeDownCast`s are really slow! `vtkArrayDownCast` fixes this issue:

```cpp
template <typename ArrayType>
void DoSomeAction(vtkAbstractArray *array)
{
  ArrayType *myArray = vtkArrayDownCast<ArrayType>(array);
  if (myArray)
  {
    // ... (do work with myArray)
  }
}
```

`vtkArrayDownCast` automatically selects `FastDownCast` when it is defined for
the ArrayType, and otherwise falls back to `SafeDownCast`. This is the
preferred array downcast method for performance, uniformity, and reliability.

## vtk::DataArrayValueRange and vtk::DataArrayTupleRange

Array dispatching relies on having templated worker code carry out some
operation. For instance, take this `vtkArrayDispatch` code that locates the
maximum value in an array:

```cpp
// Stores the tuple/component coordinates of the maximum value:
struct FindMax
{
  vtkIdType Tuple; // Result
  int Component; // Result

  FindMax() : Tuple(-1), Component(-1) {}

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    // The type to use for temporaries, and a temporary to store
    // the current maximum value:
    typedef typename ArrayT::ValueType ValueType;
    ValueType max = std::numeric_limits<ValueType>::min();

    // Iterate through all tuples and components, noting the location
    // of the largest element found.
    vtkIdType numTuples = array->GetNumberOfTuples();
    int numComps = array->GetNumberOfComponents();
    for (vtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx)
    {
      for (int compIdx = 0; compIdx < numComps; ++compIdx)
      {
        if (max < array->GetTypedComponent(tupleIdx, compIdx))
        {
          max = array->GetTypedComponent(tupleIdx, compIdx);
          this->Tuple = tupleIdx;
          this->Component = compIdx;
        }
      }
    }
  }
};

void someFunction(vtkDataArray *array)
{
  FindMax maxWorker;
  vtkArrayDispatch::Dispatch::Execute(array, maxWorker);
  // Do work using maxWorker.Tuple and maxWorker.Component...
}
```

There's a problem, though. Recall that only the arrays in
`vtkArrayDispatch::Arrays` are tested for dispatching. What happens if the
array passed into someFunction wasn't on that list?

The dispatch will fail, and `maxWorker.Tuple` and `maxWorker.Component` will be
left to their initial values of -1. That's no good. What if `someFunction` is a
critical path where we want to use a fast dispatched worker if possible, but
still have valid results to use if dispatching fails? Well, we can fall back on
the `vtkDataArray` API and do things the slow way in that case. When a
dispatcher is given an unsupported array, Execute() returns false, so let's
just add a backup implementation:

```cpp
// Stores the tuple/component coordinates of the maximum value:
struct FindMax
{ /* As before... */ };

void someFunction(vtkDataArray *array)
{
  FindMax maxWorker;
  if (!vtkArrayDispatch::Dispatch::Execute(array, maxWorker))
  {
    // Reimplement FindMax::operator(), but use the vtkDataArray API's
    // "virtual double GetComponent()" instead of the more efficient
    // "ValueType GetTypedComponent()" from vtkGenericDataArray.
  }
}
```

Ok, that works. But ugh...why write the same algorithm twice? That's extra
debugging, extra testing, extra maintenance burden, and just plain not fun.

So here is where `vtk::DataArrayValueRange` and `vtk::DataArrayTupleRange` come in.

1. `vtk::DataArrayValueRange` provides a range-based for loop interface to access the
   elements of a `vtkAbstractArray` subclass as if it's a simple flat array, like `std::vector<ValueType>`.
2. `vtk::DataArrayTupleRange` provides a similar interface, but presents the data as
   a set of tuples.

Both of these utilities work with both `vtkAbstractArray` and `vtkDataArray` concrete subclasses
and `vtkDataArray` too, using the most efficient access method available for the given array type.
abstract away the storage details of the underlying array, allowing efficient access to the data
regardless of whether the array is AOS, SOA, or some other storage scheme.

The also provide C++11 range-based for loop support, making iterating through the data easy and
less error-prone, along with functions like `size()` to get the number of values/tuples in the array.
If the tuple size is known at compile time, it can be provided as a template parameter to
`vtk::DataArrayTupleRange`, allowing for further optimizations.

`vtk::DataArrayValueRange` also has a second template parameter, `ForceValueTypeForVtkDataArray`,
which allows the user to specify the type to use when the underlying array is a
`vtkDataArray`. This is useful when the user wants to avoid the default double
type for `vtkDataArray`s. A usefully utility is `vtk::GetAPIType<ArrayType>`,
which returns the corresponding ValueType for concrete subclasses and double
for `vtkDataArray`. Both these utilities provide similar funct

Using `vtk::DataArray(Tuple/Value)Range`, we can write a single worker template that works
for both `vtkDataArray` and `vtkGenericDataArray`, without a loss of
performance in the latter case. That worker looks like this:

```cpp
// Better, uses vtk::DataArrayTupleRange:
struct FindMax
{
  vtkIdType Tuple; // Result
  int Component; // Result

  FindMax() : Tuple(-1), Component(-1) {}

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    // Create the range:
    auto range = vtk::DataArrayTupleRange(array);

    // Prepare the temporary.
    using ValueType = vtk::GetAPIType<ArrayT>;
    ValueType max = std::numeric_limits<ValueType>::min();

    vtkIdType numTuples = array->GetNumberOfTuples();
    int numComps = array->GetNumberOfComponents();
    for (auto tuple : range)
    {
      for (auto comp : tuple)
      {
        if (max < comp)
        {
          max = comp;
          this->Tuple = tupleIdx;
          this->Component = compIdx;
        }
      }
    }
  }
};
```

Now when we call `operator()` with say, `ArrayT=vtkFloatArray`, we'll get an
optimized, efficient code path. But we can also call this same implementation
with `ArrayT=vtkDataArray` and still get a correct result (assuming that the
`vtkDataArray`'s double API represents the data well enough).

Using the `vtkDataArray` fallback path is straightforward. At the call site:

```cpp
void someFunction(vtkDataArray *array)
{
  FindMax maxWorker;
  if (!vtkArrayDispatch::Dispatch::Execute(array, maxWorker))
  {
    maxWorker(array); // Dispatch failed, call vtkDataArray fallback
  }
  // Do work using maxWorker.Tuple and maxWorker.Component -- now we know
  // for sure that they're initialized!
}
```

## vtkArrayDispatch

The dispatchers implemented in the vtkArrayDispatch namespace provide array
dispatching with customizable restrictions on code generation and a simple
syntax that hides the messy details of type resolution and multi-array
dispatch. There are several "flavors" of dispatch available that operate on up
to three arrays simultaneously.

### Components Of A Dispatch

Using the `vtkArrayDispatch` system requires three elements: the array(s), the
worker, and the dispatcher.

#### The Arrays

All dispatched arrays must be concrete subclasses of `vtkAbstractArray`. It is important
to identify as many restrictions as possible. Must every ArrayType be considered
during dispatch, or is the array's ValueType (or even the ArrayType itself)
restricted? If dispatching multiple arrays at once, are they expected to have
the same ValueType? These scenarios are common, and these conditions can be
used to reduce the number of instantiations of the worker template.

#### The Worker

The worker is some generic callable. In C++98, a templated functor is a good
choice. In C++14, a generic lambda is a usable option as well. For our
purposes, we'll only consider the functor approach,, but since VTK 9.5 requires C++17,
generic lambdas should work just fine as workers, too.

At a minimum, the worker functor should define `operator()` to make it
callable. This should be a function template with a template parameter for each
array it should handle. For a three array dispatch, it should look something
like this:

```cpp
struct ThreeArrayWorker
{
  template <typename Array1T, typename Array2T, typename Array3T>
  void operator()(Array1T* array1, Array2T* array2, Array3T* array3)
  {
    /* Do stuff... */
  }
};
```

At runtime, the dispatcher will call `ThreeWayWorker::operator()` with a set of
`Array1T`, `Array2T`, and `Array3T` that satisfy any dispatch restrictions.

Workers can be stateful, too, as seen in the `FindMax` worker earlier where the
worker simply identified the component and tuple id of the largest value in the
array. The functor stored them for the caller to use in further analysis:

```cpp
// Example of a stateful dispatch functor:
struct FindMax
{
  // Functor state, holds results that are accessible to the caller:
  vtkIdType Tuple;
  int Component;

  // Set initial values:
  FindMax() : Tuple(-1), Component(-1) {}

  // Template method to set Tuple and Component ivars:
  template <typename ArrayT>
  void operator()(ArrayT *array)
  {
    /* Do stuff... */
  }
};
```

#### The Dispatcher

The dispatcher is the workhorse of the system. It is responsible for applying
restrictions, resolving array types, and generating the requested template
instantiations. It has responsibilities both at run-time and compile-time.

During compilation, the dispatcher will identify the valid combinations of
arrays that can be used according to the restrictions. This is done by starting
with a typelist of arrays, either supplied as a template parameter or by
defaulting to `vtkArrayDispatch::Arrays`, and/or filtering them by ValueType if
needed. For multi-array dispatches, additional restrictions may apply, such as
forcing the second and third arrays to have the same ValueType as the first. It
must then generate the required code for the dispatch -- that is, the templated
worker implementation must be instantiated for each valid combination of
arrays. These implementation are stored in an array and can be accessed at runtime
using `GetDataType` and `GetArrayType` with `O(1)` complexity, if their array type
is listed in the `vtkArrayTypes` enum.

At runtime, the dispatcher can either get the correct implementation with `O(1)`
complexity (most common), or if the provided are not regular VTK arrays, it tests
each of the dispatched arrays to see if they match one of the generated code paths.
Runtime type resolution is carried out using `vtkArrayDownCast` to get the best
performance available for the arrays of interest. If it finds a match, it calls
the worker's `operator()` method with the properly typed arrays. If no match is found,
it returns `false` without executing the worker.

### Restrictions: Why They Matter

We've made several mentions of using restrictions to reduce the number of
template instantiations during a dispatch operation. You may be wondering if it
really matters so much. Let's consider some numbers.

VTK is configured to use 13 ValueTypes for numeric data. These are the standard
numeric types `float`, `int`, `unsigned char`, etc. By default, VTK will define
`vtkArrayDispatch::Arrays` to use all 13 types with `vtkAOSDataArrayTemplate`
for the standard set of dispatchable arrays. If enabled during compilation, the
SOA data arrays are added to this list for a total of 26 arrays.

Using these 26 arrays in a single, unrestricted dispatch will result in 26
instantiations of the worker template. A double dispatch will generate 676
workers. A triple dispatch with no restrictions creates a whopping 17,576
functions to handle the possible combinations of arrays. That's a _lot_ of
instructions to pack into the final binary object.

Applying some simple restrictions can reduce this immensely. Say we know that
the arrays will only contain `float`s or `double`s. This would reduce the
single dispatch to 4 instantiations, the double dispatch to 16, and the triple
to 64. We've just reduced the generated code size significantly. Dispatch restriction
is a powerful tool for reducing the compiled size of a binary object.

Another common restriction is that all arrays in a multi-array dispatch have
the same ValueType, even if that ValueType is not known at compile time. By
specifying this restriction, a double dispatch on all 26 AOS/SOA arrays will
only produce 52 worker instantiations, down from 676. The triple dispatch drops
to 104 instantiations from 17,576.

Always apply restrictions when they are known, especially for multi-array
dispatches. The savings are worth it.

### Types of Dispatchers

Now that we've discussed the components of a dispatch operation, what the
dispatchers do, and the importance of restricting dispatches, let's take a look
at the types of dispatchers available.

---

#### vtkArrayDispatch::Dispatch

This family of dispatchers take no parameters and perform an unrestricted
dispatch over all arrays in `vtkArrayDispatch::Arrays`.

__Variations__:

* `vtkArrayDispatch::Dispatch`: Single dispatch.
* `vtkArrayDispatch::Dispatch2`: Double dispatch.
* `vtkArrayDispatch::Dispatch3`: Triple dispatch.

__Arrays considered__: All arrays in `vtkArrayDispatch::Arrays`.

__Restrictions__: None.

__Usecase__: Used when no useful information exists that can be used to apply
restrictions.

__Example Usage__:

```cpp
vtkArrayDispatch::Dispatch::Execute(array, worker);
```

---

#### vtkArrayDispatch::DispatchByArray

This family of dispatchers takes a `vtkTypeList` of explicit array types to use
during dispatching. They should only be used when an array's exact type is
restricted. If dispatching multiple arrays and only one has such type
restrictions, use `vtkArrayDispatch::Arrays` (or a filtered version) for the
unrestricted arrays.

__Variations__:

* `vtkArrayDispatch::DispatchByArray`: Single dispatch.
* `vtkArrayDispatch::Dispatch2ByArray`: Double dispatch.
* `vtkArrayDispatch::Dispatch3ByArray`: Triple dispatch.

__Arrays considered__: All arrays explicitly listed in the parameter lists.

__Restrictions__: Array must be explicitly listed in the dispatcher's type.

__Use case__: Used when one or more arrays have known implementations.

__Example Usage__:

An example here would be a filter that processes an input array of some
integral type and produces either a `vtkDoubleArray` or a `vtkFloatArray`,
depending on some condition. Since the input array's implementation is unknown
(it comes from outside the filter), we'll rely on a ValueType-filtered version
of `vtkArrayDispatch::Arrays` for its type. However, we know the output array
is either `vtkDoubleArray` or `vtkFloatArray`, so we'll want to be sure to
apply that restriction:

```cpp
// input has an unknown implementation, but an integral ValueType.
vtkDataArray *input = ...;

// Output is always either vtkFloatArray or vtkDoubleArray:
vtkDataArray *output = someCondition ? vtkFloatArray::New()
                                     : vtkDoubleArray::New();

// Define the valid ArrayTypes for input by filtering
// vtkArrayDispatch::Arrays to remove non-integral types:
using InputTypes = vtkArrayDispatch::FilterArraysByValueType<
  vtkArrayDispatch::Arrays, vtkArrayDispatch::Integrals>::Result;

// For output, create a new vtkTypeList with the only two possibilities:
using OutputTypes = vtkTypeList::Create<vtkFloatArray, vtkDoubleArray>;

// Typedef the dispatch to a more manageable name:
using Dispatcher = vtkArrayDispatch::Dispatch2ByArray<InputTypes, OutputTypes>;

// Execute the dispatch:
Dispatcher::Execute(input, output, someWorker);
```

---

### vtkArrayDispatch::DispatchByArrayAndValueType

This family of dispatchers takes a `vtkTypeList` of explicit array types to use
during dispatching, and a `vtkTypeList` of ValueTypes to consider during dispatch.

__Variations__:

* `vtkArrayDispatch::DispatchByArrayAndValueType`: Single dispatch.
* `vtkArrayDispatch::Dispatch2ByArrayAndValueType`: Double dispatch.
* `vtkArrayDispatch::Dispatch3ByArrayAndValueType`: Triple dispatch.

__Arrays considered__: All arrays explicitly listed in the parameter lists that also
have ValueTypes in the ValueType typelists.

__Restrictions__: Array must be explicitly listed in the dispatcher's type, and its ValueType
must be in the ValueType typelist.

__Use case__: Used when one or more arrays have known implementations, but those implementations
have unknown ValueTypes, and the ValueTypes are restricted in some way.

__Example Usage__:

The example above can also be implemented using this family of dispatchers as follows:

```cpp
// input has an unknown implementation, but an integral ValueType.
vtkDataArray *input = ...;

// Output is always either vtkFloatArray or vtkDoubleArray:
vtkDataArray *output = someCondition ? vtkFloatArray::New()
                                     : vtkDoubleArray::New();

// Define the valid ArrayTypes for input by filtering
// vtkArrayDispatch::Arrays to remove non-integral types:
using MyDispatch = vtkArrayDispatch::Dispatch2ByArrayAndValueType<
  vtkArrayDispatch::Arrays, vtkArrayDispatch::Integrals,
  vtkArrayDispatch::AOSArrays, vtkArrayDispatch::Reals>;

// Execute the dispatch:
MyDispatch::Execute(input, output, someWorker);
```

---

#### vtkArrayDispatch::DispatchByValueType

This family of dispatchers takes a vtkTypeList of ValueTypes for each array and
restricts dispatch to only arrays in vtkArrayDispatch::Arrays that have one of
the specified value types.

__Variations__:

* `vtkArrayDispatch::DispatchByValueType`: Single dispatch.
* `vtkArrayDispatch::Dispatch2ByValueType`: Double dispatch.
* `vtkArrayDispatch::Dispatch3ByValueType`: Triple dispatch.

__Arrays considered__: All arrays in `vtkArrayDispatch::Arrays` that meet the
ValueType requirements.

__Restrictions__: Arrays that do not satisfy the ValueType requirements are
eliminated.

__Use case__: Used when one or more of the dispatched arrays has an unknown
implementation, but a known (or restricted) ValueType.

__Example Usage__:

Here we'll consider a filter that processes three arrays. The first is a
complete unknown. The second is known to hold `unsigned char`, but we don't
know the implementation. The third holds either `double`s or `float`s, but its
implementation is also unknown.

```cpp
// Complete unknown:
vtkDataArray *array1 = ...;
// Some array holding unsigned chars:
vtkDataArray *array2 = ...;
// Some array holding either floats or doubles:
vtkDataArray *array3 = ...;

// Typedef the dispatch to a more manageable name:
using Dispatcher = vtkArrayDispatch::Dispatch3ByValueType<
  vtkArrayDispatch::AllTypes, vtkTypeList::Create<unsigned char>, vtkArrayDispatch::Reals>;

// Execute the dispatch:
Dispatcher::Execute(array1, array2, array3, someWorker);
```

---

#### vtkArrayDispatch::DispatchByArrayWithSameValueType

This family of dispatchers takes a `vtkTypeList` of ArrayTypes for each array
and restricts dispatch to only consider arrays from those typelists, with the
added requirement that all dispatched arrays share a ValueType.

__Variations__:

* `vtkArrayDispatch::Dispatch2ByArrayWithSameValueType`: Double dispatch.
* `vtkArrayDispatch::Dispatch3ByArrayWithSameValueType`: Triple dispatch.

__Arrays considered__: All arrays in the explicit typelists that meet the
ValueType requirements.

__Restrictions__: Combinations of arrays with differing ValueTypes are
eliminated.

__Use case__: When one or more arrays are known to belong to a restricted set of
ArrayTypes, and all arrays are known to share the same ValueType, regardless of
implementation.

__Example Usage__:

Let's consider a double array dispatch, with `array1` known to be one of four
common array types (AOS `float`, `double`, `int`, and `vtkIdType` arrays), and
the other is a complete unknown, although we know that it holds the same
ValueType as `array1`.

```cpp
// AOS float, double, int, or vtkIdType array:
vtkDataArray *array1 = ...;
// Unknown implementation, but the ValueType matches array1:
vtkDataArray *array2 = ...;

// array1's possible types:
using Array1Types =
  vtkTypeList::Create<vtkFloatArray, vtkDoubleArray, vtkIntArray, vtkIdTypeArray>;

// array2's possible types:
using Array2Types = vtkArrayDispatch::FilterArraysByValueType<
  vtkArrayDispatch::Arrays, vtkTypeList::Create<float, double, int, vtkIdType>>::Result;

// Typedef the dispatch to a more manageable name:
using Dispatcher = vtkArrayDispatch::Dispatch2ByArrayWithSameValueType<
  Array1Types, Array2Types>;

// Execute the dispatch:
Dispatcher::Execute(array1, array2, someWorker);
```

---

#### vtkArrayDispatch::DispatchBySameValueType

This family of dispatchers takes a single `vtkTypeList` of ValueType and
restricts dispatch to only consider arrays from `vtkArrayDispatch::Arrays` with
those ValueTypes, with the added requirement that all dispatched arrays share a
ValueType.

__Variations__:

* `vtkArrayDispatch::Dispatch2BySameValueType`: Double dispatch.
* `vtkArrayDispatch::Dispatch3BySameValueType`: Triple dispatch.
* `vtkArrayDispatch::Dispatch2SameValueType`: Double dispatch using
  `vtkArrayDispatch::AllTypes`.
* `vtkArrayDispatch::Dispatch3SameValueType`: Triple dispatch using
  `vtkArrayDispatch::AllTypes`.

__Arrays considered__: All arrays in `vtkArrayDispatch::Arrays` that meet the
ValueType requirements.

__Restrictions__: Combinations of arrays with differing ValueTypes are
eliminated.

__Use case__: When one or more arrays are known to belong to a restricted set of
ValueTypes, and all arrays are known to share the same ValueType, regardless of
implementation.

__Example Usage__:

Let's consider a double array dispatch, with `array1` known to be one of four
common ValueTypes (`float`, `double`, `int`, and `vtkIdType` arrays), and
`array2` known to have the same ValueType as `array1`.

```cpp
// Some float, double, int, or vtkIdType array:
vtkDataArray *array1 = ...;
// Unknown, but the ValueType matches array1:
vtkDataArray *array2 = ...;

// The allowed ValueTypes:
using ValidValueTypes = vtkTypeList::Create<float, double, int, vtkIdType>;

// Typedef the dispatch to a more manageable name:
using Dispatcher = vtkArrayDispatch::Dispatch2BySameValueType<ValidValueTypes>;

// Execute the dispatch:
Dispatcher::Execute(array1, array2, someWorker);
```

---

## Advanced Usage

### Accessing Memory Buffers

Despite the thin `vtkGenericDataArray` API's nice feature that compilers can
optimize memory accesses, sometimes there are still legitimate reasons to
access the underlying memory buffer. This can still be done safely by providing
overloads to your worker's `operator()` method. For instance,
`vtkDataArray::DeepCopy` uses a generic implementation when mixed array
implementations are used, but has optimized overloads for copying between
arrays with the same ValueType and implementation. The worker for this dispatch
is shown below as an example:

```cpp
// Copy tuples from src to dest:
struct DeepCopyWorker
{
  // AoS --> AoS same-type specialization:
  template <typename ValueType>
  void operator()(vtkAOSDataArrayTemplate<ValueType> *src,
                  vtkAOSDataArrayTemplate<ValueType> *dst)
  {
    std::copy(src->Begin(), src->End(), dst->Begin());
  }

  // SoA --> SoA same-type specialization:
  template <typename ValueType>
  void operator()(vtkSOADataArrayTemplate<ValueType>* src,
                  vtkSOADataArrayTemplate<ValueType>* dst)
  {
    vtkIdType numTuples = src->GetNumberOfTuples();
    for (int comp; comp < src->GetNumberOfComponents(); ++comp)
    {
      ValueType *srcBegin = src->GetComponentArrayPointer(comp);
      ValueType *srcEnd = srcBegin + numTuples;
      ValueType *dstBegin = dst->GetComponentArrayPointer(comp);

      std::copy(srcBegin, srcEnd, dstBegin);
    }
  }

  // Generic implementation:
  template <typename Array1T, typename Array2T>
  void operator()(Array1T* srcArray, Array2T* dstArray)
  {
    auto src = vtk::DataArrayTupleRange(srcArray);
    auto dst = vtk::DataArrayTupleRange(dstArray);

    using DestType = vtk::GetAPIType<Array2T>;

    vtkIdType tuples = src->GetNumberOfTuples();
    int comps = src->GetNumberOfComponents();
    for (vtkIdType t = 0; t < tuples; ++t)
    {
      for (int c = 0; c < comps; ++c)
      {
        dst[t][c] = static_cast<DestType>(src[t][c]);
      }
    }
  }
};
```

## Putting It All Together

Now that we've explored the new tools introduced with VTK that allow efficient,
implementation agnostic array access, let's take another look at the `calcMagnitude`
example from before and identify the key features of the implementation:

```cpp
// Modern implementation of calcMagnitude using new concepts in VTK:
struct CalcMagnitudeWorker
{
  void operator()(VectorArray* vectorsArray, MagnitudeArray* magnitudeArray)
  {
    auto vectors = vtk::DataArrayTupleRange<3>(vectorsArray);
    auto magnitude = vtk::DataArrayValueRange<1>(magnitudeArray);

    vtkIdType numVectors = vectorsArray->GetNumberOfTuples();
    for (vtkIdType tupleIdx = 0; tupleIdx < numVectors; ++tupleIdx)
    {
      // These operataors[] compile to inlined optimizable raw memory accesses
      // for vtkGenericDataArray subclasses.
      magnitude[tupleIdx] = std::sqrt(
        vectors[tupleIdx][0] * vectors[tupleIdx][0] +
        vectors[tupleIdx][1] * vectors[tupleIdx][1] +
        vectors[tupleIdx][2] * vectors[tupleIdx][2]);
    }
  }
};

void calcMagnitude(vtkDataArray *vectors, vtkDataArray *magnitude)
{
  CalcMagnitudeWorker worker;
  using Dispatcher = vtkArrayDispatch::Dispatch2ByValueType<
    vtkArrayDispatch::AllTypes, vtkArrayDispatch::Reals>;

  if (!Dispatcher::Execute(vectors, magnitude, worker))
  {
    worker(vectors, magnitude); // vtkDataArray fallback
  }
}
```

This implementation:

* Uses dispatch restrictions to reduce the number of instantiated templated
  worker functions.
* Assuming 26 types are in `vtkArrayDispatch::Arrays` (13 AOS + 13 SOA).
* The first array is unrestricted. All 26 array types are considered.
* The second array is restricted to `float` or `double` ValueTypes, which
  translates to 4 array types (one each, SOA and AOS).
* 26 * 4 = 104 possible combinations exist. We've eliminated 26 * 22 = 572
  combinations that an unrestricted double-dispatch would have generated (it
  would create 676 instantiations).
* The calculation is still carried out at `double` precision when the ValueType
  restrictions are not met.
* Just because we don't want those other 572 cases to have special code
  generated doesn't necessarily mean that we wouldn't want them to run.
* Thanks to `vtk::DataArray(Value/Tuple)Range`, we have a fallback implementation that
  reuses our templated worker code.
* In this case, the dispatch is really just a fast-path implementation for
  floating point output types.
* The performance should be identical to iterating through raw memory buffers.
* The `vtkGenericDataArray` API is transparent to the compiler. The
  specialized instantiations of `operator()` can be heavily optimized since the
  memory access patterns are known and well-defined.

## Guidelines to remove/reduce vtkAbstractArray::GetVoidPointer() usages from your codebase

As we have already mentioned, `vtkAbstractArray::GetVoidPointer()` is a function that returns
a raw pointer to the underlying data of a VTK array. This function was added in VTK's early
days when it only supported `vtkAOSDataArrayTemplate` arrays. Since then, VTK has added support
for multiple other array types, either explicit (e.g., `vtkSOADataArrayTemplate`, etc.) or
implicit (e.g., `vtkAffineArray`, `vtkConstantArray`, etc.). These array types don't store
their data in a contiguous block of memory, so using `GetVoidPointer()` leads to duplication
of data in an internal `vtkAOSDataArrayTemplate` array, which leads to memory usage increase
and unnecessary copying overhead.

The following guidelines can be used to remove the usage of `GetVoidPointer()`:

1. If array is `vtkAOSDataArrayTemplate/vtkBitArray/vtkmDataArray`, use `GetPointer()` instead of `GetVoidPointer()`.
2. If array is `vtkSOADataArrayTemplate`, and only a single component is needed, use `GetComponentArrayPointer()`
   instead of `GetVoidPointer()`.
3. If array is `vtkDataArray` and all tuples need to be filled, use `Fill` instead of `memset()` with
   `GetVoidPointer()`.
4. If array is `vtkAbstractArray` and all tuples need to be copied, use `DeepCopy()/ShallowCopy()` instead of
   `memcpy()` with `GetVoidPointer()`.
5. If array is `vtkAbstractArray` and some tuples need to be copied, use `InsertTuples*()` methods instead of
   `GetVoidPointer()` and `memcpy()`.
6. If two `vtkAbstractArray`s need to be compared, use `vtkTestUtilities::CompareAbstractArray()` instead of
   `GetVoidPointer()` and `std::equal()`.
7. If array is `vtkAbstractArray` and values are needed as `vtkVariant`, use `GetVariantValue()` instead of
   `vtkExtraExtendedTemplateMacro` with `GetVoidPointer()`.
8. If array is `vtkAbstractArray` and access to the tuples/values is needed, use `vtkArrayDispatch` and
   `vtk::DataArray(Value/Tuple)Range()`, instead of `vtkTemplateMacro` (or other variations) with `GetVoidPointer()`.
9. If array is `vtkDataArray`, the data type is known, and raw pointer to data is absolutely needed, use
   `auto aos = array->ToAOSDataArray()`, and then use `vtkAOSDataArrayTemplate<Type>::FastDownCast(aos)->GetPointer()`,
   instead of `GetVoidPointer()`, being aware that this may lead to data duplication if the array is not
   `vtkAOSDataArrayTemplate`.

The following guidelines can be used to ensure that using `GetVoidPointer()` is _safe_:

1. If array is `vtkDataArray`, but using `HasStandardMemoryLayout`, it has been verified that it actually is
   `vtkAOSDataArrayTemplate`, then `GetVoidPointer()` is _safe_ to use. This usually happens when the array was created
   using `vtkAbstractArray::CreateArray()` or `vtkDataArray::CreateDataArray()` by passing a standard VTK data type.
2. If array is `vtkDataArray`, the data type is NOT known, but raw pointer to data is absolutely needed, use
   `auto aos = array->ToAOSDataArray()`, and then use `aos->GetVoidPointer()`, which is _safe_, being aware that this
   may lead to data duplication if the array is not `vtkAOSDataArrayTemplate`.

Finally, `vtkAbstractArray::GetVoidPointer()` and `vtkDataArray::GetVoidPointer()` have been marked as an _unsafe_
function using `clang-tidy`, and its usage will be warned as such, unless explicitly marked as _safe_ using the
`(bugprone-unsafe-functions)` lint, upon guaranteeing that it is indeed safe to use, if and only if, it satisfies one of
the two conditions mentioned above.
