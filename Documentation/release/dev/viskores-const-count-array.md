## Convert constant and affine arrays to Viskores

Previously, if a dataset field was represented by a `vtkConstantArray` or a
`vtkAffineArray`, the entire conversion process would fail. The Viskores data
converters now properly convert `vtkConstantArray` to
`viskores::cont::ArrayHandleConstant` and `vtkAffineArray` to either
`viskores::cont::ArrayHandleIndex` or `viskores::cont::ArrayHandleCounting`.

This fixes problems seen in ParaView where the LOD conversion throws a warning
(and reverts to a serial algorithm) in instances like multi-block datasets.
These datasets often have constant arrays to identify blocks within each
partition.

The code is also updated to just issue a warning and continue conversion when an
array of unknown type is encountered. This means the Viskores algorithm will
still run, but the fields with "weird" array types will get dropped.
