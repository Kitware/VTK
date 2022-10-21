# New vtkImplicitArrays!

VTK now offers new flexible `vtkImplicitArray` template class that implements the `vtkGenericDataArray` interface. It essentially transforms an implicit function mapping integers to values into a pratically zero cost `vtkDataArray`. This is helpful in cases where one needs to attach data to data sets and memory efficiency is paramount.

The `vtkImplicitArray` is templated on the backend that is "duck typed" so that it can be any const functor/closure object or anything that has a `map(int) const` method. Here is a small example using a constant functor in an implicit array:

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

For convenience, a number of backends have been pre-packed into the `vtkImplicitArray` framework and can be included into the dispatch mechanism with the relevant `VTK_DISPATCH_*_ARRAYS`. They are, in no particular order:

- `vtkStdFunctionArray<ValueType>`: using a `std::function<ValueType(int)>` backend capable of covering almost any function one might want to use
- `vtkConstantArray<ValueType>`: using the `vtkConstantImplicitBackend<ValueType>` closure backend that gets constructed with a given value and then returns that same value regardless of the index queried
- `vtkAffineArray<ValueType>`: using the `vtkAffineImplicitBackend<ValueType>` closure backend that gets constructed with a slope and intercept and then returns values linearly depending on the queried index

Here is a small code snippet example to illustrate the usage of the `vtkConstantArray`:

```
vtkNew<vtkConstantArray<int>> arr42;
arr42->SetBackend(std::make_shared<vtkConstantImplicitBackend<int>>(42));
arr42->SetNumberOfComponents(1);
arr42->SetNumberOfTuples(100);
CHECK(arr42->GetValue(77) == 42); // always true
```
