# vtkX11Functions

The `VTK::x11` module provides a minimal interface to X11. It locates the X11 libraries and makes their include directories available to dependent modules. There is a `VTK_HAVE_XCURSOR` preprocessor boolean which indicates whether Xcursor support is available or not. Linking to X11 libraries is intentionally deferred to runtime, allowing projects to avoid unnecessarily depend on `libX11`, when X11 functionality is not required. This is true when using the EGL, or OSMesa render windows.
