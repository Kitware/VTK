## New vtkImplicitArrays!

### Description

VTK now offers new flexible `vtkImplicitArray` template class that implements a **read-only** `vtkGenericDataArray` interface. It essentially transforms an implicit function mapping integers to values into a practically zero cost `vtkDataArray`. This is helpful in cases where one needs to attach data to data sets and memory efficiency is paramount.

### Philosophy

In order to reduce the overhead of these containers as much as possible, `vtkImplicitArray`s are templated on a "Backend" type. This backend is "duck typed" so that it can be any const functor/closure object or anything that has a `map(int) const` method to provide a certain degree of flexibility and compute performance that is easily obtained through the static dispatching native to templates. If a `void mapTuple(vtkIdType, TupleType*) const` method is also present, the array will use this method to populate the tuple instead of the map method. As such, developers can use tried and tested backends in the VTK framework when it fits their needs and also develop their own backends on the fly for specific use cases. `vtkImplicitArray`s can then be packed into data sets to be transmitted through the data treatment pipeline with the idea that calls to the "read-only" API of `vtkGenericDataArray` should be inlined through the implicit array and generate close to zero overhead with respect to calling the backend itself.

### Usage

Here is a small example using a constant functor in an implicit array:

```
struct ConstBackend
{
  int operator()(int vtkNotUsed(idx)) const { return 42; };
};

vtkNew<vtkImplicitArray<ConstBackend>> arr42;
arr42->SetNumberOfComponents(1);
arr42->SetNumberOfTuples(100);
CHECK(arr42->GetValue(77) == 42); // always true
```

Here is a small example using map and mapTuple methods in an implicit array:

```
struct ConstTupleStruct
{
  int Tuple[3] = { 0, 0, 0 };

  ConstTupleStruct(int tuple[3])
  {
    this->Tuple[0] = tuple[0];
    this->Tuple[1] = tuple[1];
    this->Tuple[2] = tuple[2];
  }

  // used for GetValue
  int map(int idx) const
  {
    int tuple[3];
    this->mapTuple(idx / 3, tuple);
    return tuple[idx % 3];
  }
  // used for GetTypedTuple
  void mapTuple(int vtkNotUsed(idx), int* tuple) const
  {
    tuple[0] = this->Tuple[0];
    tuple[1] = this->Tuple[1];
    tuple[2] = this->Tuple[2];
  }
};

vtkNew<vtkImplicitArray<ConstTupleStruct>> mapTupleArr;
int tuple[3] = { 1, 2, 3 };
mapTupleArr->ConstructBackend(tuple);
mapTupleArr->SetNumberOfComponents(3);
mapTupleArr->SetNumberOfTuples(100);
int tuple[3] = { 0, 0, 0 };
mapTupleArr->GetTypedTuple(77, tuple);
CHECK(tuple[0] == 1 && tuple[1] == 2 && tuple[2] == 3); // always true
```

For convenience, a number of backends have been pre-packed into the `vtkImplicitArray` framework. They are, in alphabetical order:

- `vtkAffineArray<ValueType>`: using the `vtkAffineImplicitBackend<ValueType>` closure backend that gets constructed with a slope and intercept and then returns values linearly depending on the queried index
- `vtkCompositeArray<ValueType>`: using the `vtkCompositeImplicitBackend<ValueType>` closure backend that takes an `std::vector<vtkDataArray*>` at construction and returns values as if the list has been concatenated into one array
- `vtkConstantArray<ValueType>`: using the `vtkConstantImplicitBackend<ValueType>` closure backend that gets constructed with a given value and then returns that same value regardless of the index queried
- `vtkStdFunctionArray<ValueType>`: using a `std::function<ValueType(int)>` backend capable of covering almost any function one might want to use
- `vtkIndexedArray<ValueType>`: using the `vtkIndexedImplicitBackend<ValueType>` closure backend that takes an indexing array (either `vtkIdList` or `vtkDataArray`) and a base `vtkDataArray` at construction and returns values indirected using the indexing array to give access to a shuffled array without the memory cost

Here is a small code snippet example to illustrate the usage of the `vtkConstantArray`:

```
vtkNew<vtkConstantArray<int>> arr42;
arr42->ConstructBackend(42);
arr42->SetNumberOfComponents(1);
arr42->SetNumberOfTuples(100);
CHECK(arr42->GetValue(77) == 42); // always true
```

Multi-component arrays have de-facto AOS ordering with respect to the backend.

The read-only parts of the `vtkDataArray` API work out of the box for any `vtkImplicitArray` including the following functionalities:
  * Get components, typed components, tuples, number of components and number of tuples
  * `vtkImplicitArray::GetVoidPointer` (implementing and internal deep copy of the array into an AOS array that can be destroyed with `vtkImplicitArray::Squeeze`)
  * Standard library like ranges and iterators using `vtkDataArrayRange` functionalities
  * `vtkArrayDispatch`, provided the correct compilation options have been set and the correct type list is used for dispatching (see below)

### Focus on `vtkCompositeArrays`

The `vtkCompositeArray` is a family of `vtkImplicitArray`s that can concatenate arrays together to interface a group of arrays as if they were a single array. This concatenation operates in the "tuple" direction and not in the "component" direction.

This new array relies on the `vtkCompositeImplicitBackend` template class to join an `std::vector` of `vtkDataArray`s. The `vtkCompositeArray`s use internal address referencing and indirection to implement binary search on the indexes of the composite array leading to access with $O(log_2(m))$ time where $m$ is the number of leaves (or base `vtkDataArray`s) composing the composite.

To facilitate the creation of `vtkCompositeArray`s in practice, a templated utility function `vtkCompositeArrayUtilities::Concatenate` has been made available to users that can take an `std::vector` of `vtkDataArray`s (each with the same number of components) and turn them into a single concatenated `vtkCompositeArray` with the same number of components as the base array and a number of tuples being the sum of all the base arrays tuples.

A code snippet using this type of array:
```
std::vector<vtkDataArray*> baseArrays(16);
vtkNew<vtkDoubleArray> baseArray;
baseArray->SetNumberOfComponents(3);
baseArray->SetNumberOfTuples(10);
baseArray->Fill(0.0);

std::fill(baseArrays.begin(), baseArrays.end(), baseArray);
vtkSmartPointer<vtkCompositeArray<double>> composite = vtkCompositeArrayUtilities::Concatenate<double>(baseArrays); // nTuples = 160

CHECK(composite->GetComponent(42, 1) == 0.0); // always true
```

> **WARNINGS**
>
>   * Any two arrays composited into a `vtkCompositeArray` using the `vtk::ConcatenateDataArrays` method must have the same number of components.
>   * Iteration over the composited array incurs a lot of overhead compared to an explicit memory array (~3x slower with only 1 level). The use case is truly when memory efficiency is more important than compute performance
>   * This array has no relationship with the `VTKCompositeDataArray` present in the `numpy_interface.dataset_adapter` module of the python wrapping of VTK

### Focus on `vtkIndexedArray`

The family of `vtkIndexedArray`s allow you to wrap an existing `vtkDataArray` with a layer of indirection through a list of indexes (`vtkIdList` or another `vtkDataArray`) to create a derived subset data array without any excess memory consumption. As such, by providing a `vtkIndexedImplicitBackend` with an indexation array and a `vtkDataArray`, one can effectively construct a reduced and reordered view of the base array.

While using this type of feature to create only one indexed array can be counter productive (allocation of the index array more expensive than an explicit copy of the data might be), using this feature you can share the same index list amoungst multiple indexed arrays effectively using less memory total.

Here is an example use case:
```
vtkNew<vtkIntArray> baseArr;
baseArr->SetNumberOfComponents(3);
baseArr->SetNumberOfComponents(100);
auto range = vtk::DataArrayValueRange<3>(baseArr);
std::iota(range.begin(), range.end(), 0);

vtkNew<vtkIdList> indexes;
indexes->SetNumberOfIds(30);
for (idx = 0; idx < 30; idx++)
{
    indexes->SetId(ids, 10*idx);
}

vtkNew<vtkIndexedArray<int>> indexed;
indexed->SetBackend(std::make_shared<vtkIndexedImplicitBackend<int>>(indexes, baseArr));
indexed->SetNumberOfComponents(1);
indexed->SetNumberOfComponents(indexes->GetNumberOfIds());
CHECK(indexed->GetValue(13) == 130); // always true
```

> **WARNINGS**
>
>   * Significant access performance hits can be incurred due to cache missing cause by the inherent indirection in the array.

### Implementing a `vtkImplicitArray` in VTK

Implementing a new `vtkImplicitArray` in the VTK library usually passes through the following steps:
  * Implementing the backend: this is the step where the underlying functionality of the implicit array needs to be developed.
  * Creating an alias and instantiating the concrete types: once the backend is ready, one can create an alias for the array in its own header (`using MySuperArray = vtkImplicitArray<MySuperBackend>`) and use this header and the au    tomatic instantiation mechanisms present in `Common/ImplicitArrays/CMakeLists.txt` to make sure that the vtk library gets shipped with pre-compiled objects for your arrays.
  * Hooking up the dispatch mechanism: this step ensures that your new arrays can be included in the `vtkArrayDispatch` mechanism when the correct compilation option is set.
  * Testing: be sure to set up some tests in order to both verify that your implementation is functioning correctly and to avoid regressions in the future.

An example merge request that was used for including the `vtkIndexedArray`s can be found here: [!9703](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/9703)

### Building and Dispatch

The entire implicit array framework is included in the `CommonImplicitArrays` module. Support for dispatching implicit arrays can be enforced by including type lists from `vtkArrayDispatchImplicitTypeList.h` and compiling VTK with the correct `VTK_DISPATCH_*_ARRAYS` option.

## vtkToImplicitArrayFilter: compress explicit memory arrays into a `vtkImplicitArray`s

You now have access to a new filter `vtkToImplicitArrayFilter` in a new `FiltersReduction` module that can transform explicit memory arrays into `vtkImplicitArray`s. The filter itself delegates the "compression" method to a `vtkToImplicitStrategy` object using a strategy pattern.

The following strategies (in order of complexity) have been implemented so far:
- `vtkToConstantArrayStrategy`: transform an explicit array that has the same value everywhere into a `vtkConstantArray`
- `vtkToAffineArrayStrategy`: transform an explicit array that follows an affine dependence on its indexes into a `vtkAffineArray`
- `vtkToImplicitTypeErasureStrategy`: transform an explicit integral array (with more range in its value type than necessary for describing it) into a reduced memory explicit integral array wrapped in an implicit array.
- `vtkToImplicitRamerDouglasPeuckerStrategy`: transform an explicit memory array into a `vtkCompositeArray` with constant (`vtkConstantArray`) and affine (`vtkAffineArray`) parts following a Ramer-Douglas-Peucker algorithm.

> **WARNINGS**
>
>   * The CommonImplicitArrays module does not currently compile with gcc4.8.5 due to its partial c++11 support.
