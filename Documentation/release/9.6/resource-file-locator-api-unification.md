## Unify library lookup APIs in `vtkResourceFileLocator`

VTK now uses a single method to determine which library provides a given function. You can call:

vtkResourceFileLocator::GetLibraryPathForAddress(const void* ptr)

on all platforms. This replaces the previous symbol-based functions on Unix and Windows, which are now deprecated.

The vtkGetLibraryPathForSymbol(func) macro now forwards to GetLibraryPathForAddress(&func). Existing code continues to work without modification.

VTK also provides a cross-platform API to get the path of the current executable:

std::string exePath = vtkResourceFileLocator::GetCurrentExecutablePath();

This replaces the old Windows-specific trick of passing nullptr to GetLibraryPathForSymbolWin32 and works reliably on Windows, Linux, and macOS.

This change removes platform-specific behavior and improves the reliability of locating libraries and resources at runtime.
