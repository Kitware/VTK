## Flow Paths filters: changed from using vtkCharArray to vtkSignedCharArray

Changed the FlowPaths and ParallelFlowPaths filters to use `vtkSignedCharArray`
instead of `vtkCharArray`. This is because `vtkCharArray` is ambiguous in its
underlying data type (the data type is either signed or unsigned depending
on the system). Additionally the `vtkCharArray` may be converted to
`vtkSignedCharArray` or `vtkUnsignedCharArray` through file IO and if this
happens doing a `vtkCharArray::SafeDownCast()` on the pointer will return
a null pointer which can also cause unexpected problems.

This change does not modify any public member functions of these classes but does
modify some protected member functions of these classes by now returning
`vtkSignedCharArray*` for the `ParticleSourceId` point data array
instead of a `vtkCharArray*` point data array. Additionally, on output
the `vtkPolyData` will produce the "ParticleSourceId" point id array as
a `vtkSignedCharArray` instead of a `vtkCharArray`. In summary, to adjust
to this change modify code uses the `GetParticleSourceIds()` member method
or `ParticleSourceIds` member variable to be `vtkSignedArray` instead of
`vtkCharArray`. Similarly for output from any of these filters the
"ParticleSourceId" point id array should now be cast to a
`vtkSignedCharArray` instead of a `vtkCharArray`.
