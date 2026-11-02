## Render Window minimum size

Added a `vtkWindow::SetMinimumSize()` API to set the minimum window size.
Supported for OpenGL render windows on macOS, Windows and Linux (X11).
On macOS and Linux, it must be called before the window is initialized.
