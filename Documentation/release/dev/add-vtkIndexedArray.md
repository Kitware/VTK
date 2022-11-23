## New `vtkIndexedArray`s

VTK now provides a family of data arrays called `vtkIndexedArray`s. These arrays are part of the `vtkImplicitArray` framework. They allow you to wrap an existing `vtkDataArray` with a layer of indirection through a list of indexes (`vtkIdList` or another `vtkDataArray`) to create a derived subset data array without any excess memory consumption. As such, by providing a `vtkIndexedImplicitBackend` with an indexation array and a `vtkDataArray`, one can effectively construct a reduced and reordered view of the base array.

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
