## Added checking for corrupt data in `vtkCellArray` `ImportLegacyFormat` and `AppendLegacyFormat`

Two `vtkCellArray` public API, `ImportLegacyFormat` and `AppendLegacyFormat`, were changed from returning void to return bool.

If the data is corrupt, false is now returned. This allow various file reader classes to detect corrupt data instead of crashing.
