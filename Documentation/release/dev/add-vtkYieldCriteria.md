## Add vtkTensorPrincipalInvariants and vtkYieldCriteria filters

The new `vtkTensorPrincipalInvariants` filter computes principal values and vectors
from 2D and 3D symmetric tensors (described with arrays containing 3 and 6
components, respectively).

The new `vtkYieldCriteria` filter computes different yield criteria from given
2D or 3D symmetric tensors.

Available yield criteria currently include:
- Tresca criterion
- Von Mises criterion
