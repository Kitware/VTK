#Fix nearest-neighbour interpolation in vtkDataSetAttributes

In vtkDataSetAttributes.cxx and vtkDataSetAttributesList.cxx, the local variable maxWeight to find the cell vertex with maximum weight was wrongly declared as vtkIdType, instead of double. Since weight is always in [0, 1], this kept the maxWeight always as 0 and the last point of the cell was always wrongly selected. This is now corrected.
