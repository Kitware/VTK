## Fix position/size with minimized window on Windows

On Windows, when minimized, `vtkRenderWindow` APIs `GetPosition()` and `GetSize()` now return cached value instead of incorrect values.
