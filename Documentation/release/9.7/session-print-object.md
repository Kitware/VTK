# Print Objects To String in Standalone Sessions

The `vtkStandaloneSession` can now print information about VTK objects directly to a string. This is useful for debugging and logging purposes.

1. The C API of `vtkSession.h` now includes the `vtkSessionPrintObjectToString` function that allows you to print a VTK object's information to a string.
2. The `vtkStandaloneSession::PrintObjectToString` method makes object printing available in WebAssembly standalone sessions.
3. The JavaScript API of `vtkStandaloneSession` has been updated to include the `printObjectToString` method, enabling you to retrieve the printed information of a VTK object as a string in JavaScript.
