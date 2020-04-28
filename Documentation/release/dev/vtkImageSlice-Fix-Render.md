## Fix `vtkImageSlice` Render when mapper doesn't have an input

Fix `vtkImageSlice` Render which would SEGFAULT when input was empty.
