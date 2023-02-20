## Make CellTreeLocator's max depth variable

You can now customize CellTreeLocator's max depth.
Default is set to 32 as before and must be non-zero positive.

```cpp
vtkNew<vtkCellTreeLocator> locator;
locator->SetCelltreeMaxDepth(64);
```
