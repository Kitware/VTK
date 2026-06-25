## Fix vtkGenericCell DeepCopy and ShallowCopy

vtkGenericCell DeepCopy and ShallowCopy used to only forward calls
to the internal cell, which is incorrect. They now properly handle different
types and also support copying another generic cell.
