## vtkFieldData performances

Copying vtkFieldData structures with `DeepCopy` and `PassData` is now faster.
Those methods now use `SetArray` when possible instead of `AddArray`
to avoid the string comparison over existing array names (unicity check).

With `DeepCopy` and `PassData`, this unicity check was done for every source array.

The `DeepCopy` method now assume unicity in the source vtkFieldData, which allows
to `SetArray` and bypass the duplication check.

With the `PassData` method, previous arrays are kept: duplication can occur with them,
but again unicity is assumed in the source. So comparison is done only against
pre-existing arrays. Also a structure is created to speedup the comparison.

With 10000 single-tuples numeric arrays, we observed a x70 speedup for `DeepCopy`
and x180 speedup for `PassData`.
