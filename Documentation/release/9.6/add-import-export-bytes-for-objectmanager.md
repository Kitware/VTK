## Add functions to import/export object manager state from/to byte arrays


VTK's `vtkObjectManager` class now includes methods to import and export its state using byte arrays. This enhancement allows for easier serialization and deserialization of the object manager's state, facilitating tasks such as saving to disk or transmitting over a network without relying on file I/O.

### New Methods
- `std::vector<vtkSmartPointer<vtkObjectBase>> ImportFromBytes(vtkSmartPointer<vtkUnsignedCharArray> byteArray)`
- `vtkSmartPointer<vtkUnsignedCharArray> ExportToBytes()`

Please refer to the `vtkObjectManager` documentation for more details on how to use these new methods.
