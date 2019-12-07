vtk_module_third_party_external(
  PACKAGE       MPI
  COMPONENTS    C
  TARGETS       MPI::MPI_C
  INCLUDE_DIRS  MPI_C_INCLUDE_DIRS MPI_C_INCLUDE_PATH
  LIBRARIES     MPI_C_LIBRARIES
  DEFINITIONS   MPI_C_COMPILE_DEFINITIONS
  STANDARD_INCLUDE_DIRS)

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtk_mpi.h")

vtk_module_definitions(VTK::mpi
  INTERFACE
    MPICH_SKIP_MPICXX
    OMPI_SKIP_MPICXX
    _MPICC_H)

if (APPLE AND TARGET MPI::MPI_C)
  # On macOS, MPI might have the `-Wl,-flat_namespace` option in it (mpich has
  # been observed). This is incompatible with `@rpath` usage, so if we see it,
  # just remove it from the link flags of the target.
  get_property(mpi_link_flags
    TARGET    MPI::MPI_C
    PROPERTY  INTERFACE_LINK_LIBRARIES)
  set(fixed_mpi_link_flags)
  foreach (mpi_link_flag IN LISTS mpi_link_flags)
    string(FIND "${mpi_link_flag}" "-Wl,-flat_namespace" idx)
    if (idx EQUAL "-1")
      list(APPEND fixed_mpi_link_flags
        "${mpi_link_flag}")
    endif ()
  endforeach ()
  set_property(TARGET MPI::MPI_C
    PROPERTY
      INTERFACE_LINK_LIBRARIES "${fixed_mpi_link_flags}")
endif ()
