## Introduce vtkFieldDataToDataSetAttribute

With the new `vtkFieldDataToDataSetAttribute` filter VTK provides now
a way to efficiently pass FieldData single-value arrays to other AttributeData.

This is for instance useful with composite data, where FieldData
can be used to store a single scalar, varying at block level only.
This scalar was mostly not usable for computation in other filters.
Moving it, for instance, to PointData, allows to use it in your pipeline.

This is done at low memory cost by using the ImplicitArrays design.
