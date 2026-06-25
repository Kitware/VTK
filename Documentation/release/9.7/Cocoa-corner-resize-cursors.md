## macOS now supports corner resize mouse cursors

vtkCocoaRenderWindow and vtkCocoaHardwareWindow didn't support corner resize cursors (like VTK_CURSOR_SIZENE) because macOS didn't provide such cursors. Starting with macOS 15 Sequoia, such cursors are now provided by NSCursor and so VTK now uses them on macOS 15 and later.
