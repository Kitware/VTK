# vtkAxisActor and vtkPolarAxesActor breaking changes

Protected members of `vtkAxisActor` and `vtkPolarAxesActor` classes were moved to private.
Please use appropriate Getter/Setter to use them instead.

Also some API were ported from `const char*` to `std::string`.
While this is straigtforward for setters, this breaks the following getters:
- `vtkAxisActor::GetTitle()`
- `vtkPolarAxesActor::GetPolarAxisTitle()`
