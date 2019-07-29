vtk_module_find_package(
  PACKAGE ADIOS2
  VERSION 2.4)

set (classes_mpi )
set (sources_mpi )
set (private_headers_mpi )
if(ADIOS2_HAVE_MPI)
  set(classes_mpi
    vtkADIOS2VTXReader
  )

  set(sources_mpi
    vtkADIOS2VTXReader.cxx

    VTX/common/VTXDataArray.cxx
    VTX/common/VTXHelper.cxx VTX/common/VTXHelper.txx

    VTX/VTXSchemaManager.cxx

    VTX/schema/VTXSchema.cxx VTX/schema/VTXSchema.txx
    VTX/schema/vtk/VTXvtkBase.cxx
    VTX/schema/vtk/VTXvtkVTI.cxx VTX/schema/vtk/VTXvtkVTI.txx
    VTX/schema/vtk/VTXvtkVTU.cxx VTX/schema/vtk/VTXvtkVTU.txx
  )

  set(private_headers_mpi
    VTX/common/VTXTypes.h
    VTX/common/VTXDataArray.h
    VTX/common/VTXHelper.h

    VTX/VTXSchemaManager.h

    VTX/schema/VTXSchema.h
    VTX/schema/vtk/VTXvtkBase.h
    VTX/schema/vtk/VTXvtkVTI.h
    VTX/schema/vtk/VTXvtkVTU.h
  )
endif()

if (VTK_USE_MPI AND NOT ADIOS2_HAVE_MPI)
  message(FATAL_ERROR "VTK built with MPI requires ADIOS2 built with MPI")
elseif(NOT VTK_USE_MPI AND ADIOS2_HAVE_MPI)
  message(FATAL_ERROR "VTK built without MPI requires ADIOS2 built without MPI")
endif()

set(classes
  ${classes_mpi}
  vtkADIOS2CoreImageReader)

set(sources
  ${sources_mpi})

# Exlcude from wrapping
set(private_classes
  Core/vtkADIOS2CoreArraySelection)

set(private_headers
  ${private_headers_mpi}
  Core/vtkADIOS2CoreTypeTraits)

vtk_module_add_module(VTK::IOADIOS2
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers})
vtk_module_link(VTK::IOADIOS2
  PRIVATE
    adios2::adios2)

if (ADIOS2_HAVE_MPI)
  vtk_module_definitions(VTK::IOADIOS2
    PRIVATE IOADIOS2_HAVE_MPI)
endif()
