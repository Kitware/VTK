vtk_module_third_party(cosmo
  LIBRARIES vtkcosmo
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
  NO_ADD_SUBDIRECTORY)

if (VTK_USE_SYSTEM_COSMO)
  return()
endif()

# Cosmo needs MPI support.
include(vtkMPI)

## Set cosmo sources
set(${vtk-module}_SRCS
  BHTree.cxx
  bigchunk.cxx
  ChainingMesh.cxx
  CosmoHaloFinder.cxx
  CosmoHaloFinderP.cxx
  dims.cxx
  FOFHaloProperties.cxx
  GridExchange.cxx
  HaloCenterFinder.cxx
  HaloFinderInput.cxx
  InitialExchange.cxx
  Message.cxx
  ParticleDistribute.cxx
  ParticleExchange.cxx
  Partition.cxx
  SODHalo.cxx
  SubHaloFinder.cxx
# Timer.cxx
# Timings.cxx
  )

# Since Timer.cxx/Timings.cxx have portability issues, we disable them.
set (COSMO_DISABLE_TIMINGS TRUE)
configure_file(Definition.h.in Definition.h)

set (${vtk-module}_HDRS
  ${CMAKE_CURRENT_BINARY_DIR}/Definition.h)
vtk_add_library(${vtk-module} ${${vtk-module}_SRCS})
vtk_mpi_link(${vtk-module})
