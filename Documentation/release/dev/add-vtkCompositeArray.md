# `vtkCompositeArray`: a new implicit array that concatenates other arrays together

`vtkCompositeArray` is now available in VTK! It is a family of `vtkImplicitArray`s that can concatenate arrays together to interface a group of arrays as if they were a single array.

This new array relies on the `vtkCompositeImplicitBackend` template class to join `vtkDataArray`s two at a time. Creating a hiearchy of `vtkCompositeArray`s can generate a binary tree on the indexes of the composite array leading to access with $O(log_2(m))$ time where $m$ is the number of leaves (or base `vtkDataArray`s) composing the composite (or alternatively $O(l)$ where $l$ is the number of levels in the tree).

To facilitate the creation of `vtkCompositeArray`s in practice, a templated utility function `vtkCompositeArrayUtilities::Concatenate` has been made available to users that can take an `std::vector` of `vtkDataArray`s and turn them into a single concatenated `vtkCompositeArray` whose tree structure should be balanced with respect to number of arrays (a possible improvement would be to balance with respect to number of tuples following a "Huffman coding" approach).

A code snippet using this type of array:
```
std::vector<vtkDataArray*> baseArrays(16); // 16 == 2^4, will make 4 levels in binary tree
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
>   * Any two arrays composited into a `vtkCompositeArray` must have the same number of components.
>   * Iteration over the composited array incurs a lot of overhead compared to an explicit memory array (~3x slower with only 1 level). The use case is truly when memory efficiency is more important than compute performance.
