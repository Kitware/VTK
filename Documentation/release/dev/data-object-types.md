## Improvements to vtkDataObjectTypes

vtkDataObjectTypes adds two new static methods that can be used to
understand relationships between different data object types using their
type ids.

`vtkDataObjectTypes::TypeIdIsA` lets us defermine if a type is same as or a
specialization of another type.

`vtkDataObjectTypes::GetCommonBaseTypeId` lets us find the type for the first
common base-class of the two given types.
