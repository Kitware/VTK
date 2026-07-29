## DPI detection improvements

`DetectDPI()` API now works on macOS, X11 and WebAssembly render windows (it was only supported on Windows before).
Moreover, any change in DPI at runtime invokes a `vtkCommand::DPIChangedEvent` (only relevent on macOS and Windows).
