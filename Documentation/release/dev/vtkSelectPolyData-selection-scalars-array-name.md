## Define vtkSelectPolyData's Selection Scalars Array

vtkSelectPolyData have the following improvements:

1) Fixed an issue when no selection scalars array would be added to the output when GenerateSelectionScalars was set to true.

2) If GenerateSelectionScalars is enabled, an array name of the selection scalars can now be defined.

3) Now passes cell data attributes to the selected and unselected outputs.

4) Now passes point data scalars to the selected output when GenerateSelectionScalars is true.
