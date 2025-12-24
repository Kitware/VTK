## Introduce the implicit vtkStridedArray: a strided view on a buffer

The `vtkStridedArray` is a `vtkImplicitArray` providing a strided view on an existing memory buffer.

If you work with a multi-dimensional buffer of data, you can create a `vtkStridedArray` to manipulate
a single dimension (that can be itself a scalar or a vector) of this buffer through
the classical `vtkDataArray` API.

This is typically useful in the InSitu context as with the `vtkConduitSource`
for Catalyst. In this case, we want to create `vtkDataArray` around existing memory
with zero copy.

Let's have this buffer
```
std::vector<float> localBuffer
{
      0, 1000, 2000, 3000,
      1, 1001, 2001, 3001,
      2, 1002, 2002, 3002,
      3, 1003, 2003, 3003,
      4, 1004, 2004, 3004,
      5, 1005, 2005, 3005,
      6, 1006, 2006, 3006,
      7, 1007, 2007, 3007,
      8, 1008, 2008, 3008,
      9, 1009, 2009, 3009
}
```
This may represent a one-component array (first column) interlaced with a 3-components array, both with 10 tuples.

We can now create a `vtkStridedArray` as follow:
```
  vtkNew<vtkStridedArray<float>> stridedArray;
  stridedArray->SetNumberOfComponents(3);
  stridedArray->SetNumberOfTuples(10);
  stridedArray->ConstructBackend(
    localBuffer.data(), /*stride*/ 3, /*offset*/ 1, /*components*/ 3);
```
This will provide a (zero copy) view on the last 3 columns of our initial buffer.

In the context of `CatalystConduit`, with the same buffer we can write the following:
```
  fields["vectorArray/values/x"].set_external(
    localBuffer.data(), nbPts, /*offset=*/sizeof(float), stride * sizeof(float));
  fields["vectorArray/values/y"].set_external(
    localBuffer.data(), nbPts, /*offset=*/sizeof(float) * 2, stride * sizeof(float));
  fields["vectorArray/values/z"].set_external(
    localBuffer.data(), nbPts, /*offset=*/sizeof(float) * 3, stride * sizeof(float));
```

The conduit source will then provide a `vtkStridedArray` as in previous example.
