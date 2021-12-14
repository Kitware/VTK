## `vtkSet*Override` macros

The `vtkSetGet.h` macros which create setters now have `*Override` variants
that use `override` instead of plain `virtual` to ensure that they are actually
creating overrides of baseclass methods. This helps to avoid
`-Winconsistent-missing-override` warnings as well.
