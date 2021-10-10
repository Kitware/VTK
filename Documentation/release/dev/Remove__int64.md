## Removed deprecated vtkVariant API Is__Int64 and IsUnsigned__Int64

They caused -Wreserved-identifier warnings due to the double underscores.  In any case, they have been unconditionally return false for years.
