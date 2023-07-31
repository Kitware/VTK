## vtkMemoryResourceStream can now own streamed buffer

vtkMemoryResourceStream can now own streamed buffer, meaning you can free the source buffer after setting it.
You can now set source buffer as a `std::string`, a `std::vector` or a `vtkBuffer*`.
