## Define vtkSelectPolyData's Selection Scalars Array

vtkSelectPolyData have the following improvements:

1) Fixed an issue when no selection scalars array would be added to the output when GenerateSelectionScalars was set to true.

2) If GenerateSelectionScalars is enabled, an array name of the selection scalars can now be defined.
