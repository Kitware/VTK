vtk_module_test_data(
  Data/box-noglom.ex2
  Data/can.ex2
  Data/edgeFaceElem.exii)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/cube-1.exo
    Data/cyl_with_NaN.g
    Data/dodecahedron.exo
    Data/shared_face_polyhedra.exo)

  add_subdirectory(Python)
endif ()
