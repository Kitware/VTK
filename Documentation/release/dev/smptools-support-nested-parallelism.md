# Support nested parallelism

Add official support of nested parallelism for vtkSMPTools.
Several methods are added:
  - `SetNestedParallelism` takes a boolean to activate nested parallelism. (default on false)
  - `GetNestedParallelism` return true if nested parallelism is enabled.
  - `IsParallelScope` return true is it is called from a parallel scope.
