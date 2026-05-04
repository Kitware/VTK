## Fix vtkSSAAPass with multiple viewports

Fixed `vtkSSAAPass` not rendering in all viewports except bottom-left. The issue was resolved by respecting the viewport's origin when copying the result to the output framebuffer.
