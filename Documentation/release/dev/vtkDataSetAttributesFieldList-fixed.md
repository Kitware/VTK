## Fix SIGSEGV on vtkCompositeDataProbeFilter() (#17690)

vtkDataSetAttributesFieldList::vtkInternals.NumberOfInputs was
initialized to be -1, which causes UnionFieldList to pass wrong
argument to PreExtendForUnion.  It is now fixed to be 0.
