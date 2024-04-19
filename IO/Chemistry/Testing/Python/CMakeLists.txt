vtk_add_test_python(
  TestXYZMol.py
  caffeine.py
  caffeine_notrailingwhitespaces.py
)

# Test is not passing on Windows because of a CRLF/LF issue:
# https://gitlab.kitware.com/vtk/vtk/-/issues/18213
if (NOT WIN32)
  vtk_add_test_python(
    MoleculeXYZ.py,NO_VALID
  )
endif()
