# Fix AMReX particle reader for files without top-level header

The `vtkAMReXParticlesReader` now continues to read the particles even when a
top level header does not exist. Normally, the reader obtained the data time step
from the top level header. It initializes the time step to 0 if that header file
does not exist.
