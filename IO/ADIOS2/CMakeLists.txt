vtk_module_find_package(
  PACKAGE ADIOS2
  VERSION 2.4)

set (classes_mpi )
set (sources_mpi )
set (private_headers_mpi )
if(ADIOS2_HAVE_MPI)
  set(classes_mpi
    vtkVARMultiBlock
  )

  set(sources_mpi
    vtkVARMultiBlock.cxx

    VAR/common/VARDataArray.cxx
    VAR/common/VARHelper.cxx VAR/common/VARHelper.txx

    VAR/VARSchemaManager.cxx

    VAR/schema/VARSchema.cxx VAR/schema/VARSchema.txx
    VAR/schema/vtk/VARvtkBase.cxx
    VAR/schema/vtk/VARvtkVTI.cxx VAR/schema/vtk/VARvtkVTI.txx
    VAR/schema/vtk/VARvtkVTU.cxx VAR/schema/vtk/VARvtkVTU.txx
  )

  set(private_headers_mpi
    VAR/common/VARTypes.h
    VAR/common/VARDataArray.h
    VAR/common/VARHelper.h

    VAR/VARSchemaManager.h

    VAR/schema/VARSchema.h
    VAR/schema/vtk/VARvtkBase.h
    VAR/schema/vtk/VARvtkVTI.h
    VAR/schema/vtk/VARvtkVTU.h
  )
endif()

if ((NOT ADIOS2_HAVE_MPI) AND (VTK_USE_MPI))
  message(FATAL_ERROR "ADIOS2 disables MPI But VTK has MPI enabled.
                       The two should be consistent")
elseif((ADIOS2_HAVE_MPI) AND (NOT VTK_USE_MPI))
  message(FATAL_ERROR "ADIOS2 enables MPI But VTK has MPI disabled.
                       The two should be consistent")
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
