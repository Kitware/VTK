# Find the GhostScript executable for GL2PS tests.
find_program(VTK_GHOSTSCRIPT_EXECUTABLE gs gswin32c gsos2)
mark_as_advanced(VTK_GHOSTSCRIPT_EXECUTABLE)
