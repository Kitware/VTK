## Improved data array serialization performance

Data array serialization now exits early when the array's modification
time (MTime) is older than the MTime recorded in the serialization state,
avoiding redundant work. You will now notice that `vtkObjectManager::UpdateStatesFromObjects()`
runs faster when many arrays in your scene are unchanged.
