# Fix issue with Fides not initializing HIP before using it

There was an issue with Fides crashing because it attempted to allocate memory
with HIP before HIP was initialized. (See
https://gitlab.kitware.com/vtk/fides/-/issues/26.) The problem was actually with
VTK-m. (Initializing VTK-m is supposed to be optional, but HIP was not getting
initialized in time if it was not.) To fix the Fides issue, VTK-m is updated to
the changes provided by
https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/3289.
