vtk_module_find_package(
  PACKAGE ADIOS2
)

if(NOT ADIOS2_HAVE_MPI)
  message(FATAL_ERROR "The VTK::IOADIOS2 module requires ADIOS2 with MPI support")
endif()

set(classes
  vtkVARMultiBlock
)

set(sources
  vtkVARMultiBlock.cxx

  VAR/common/VARDataArray.cxx
  VAR/common/VARHelper.cxx VAR/common/VARHelper.txx

  VAR/VARSchemaManager.cxx

  VAR/schema/VARSchema.cxx VAR/schema/VARSchema.txx
  VAR/schema/vtk/VARvtkBase.cxx
  VAR/schema/vtk/VARvtkVTI.cxx VAR/schema/vtk/VARvtkVTI.txx
  VAR/schema/vtk/VARvtkVTU.cxx VAR/schema/vtk/VARvtkVTU.txx
)

set(private_headers
  VAR/common/VARTypes.h
  VAR/common/VARDataArray.h
  VAR/common/VARHelper.h

  VAR/VARSchemaManager.h

  VAR/schema/VARSchema.h
  VAR/schema/vtk/VARvtkBase.h
  VAR/schema/vtk/VARvtkVTI.h
  VAR/schema/vtk/VARvtkVTU.h
)

vtk_module_add_module(VTK::IOADIOS2
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers}
)

vtk_module_link(VTK::IOADIOS2
  PRIVATE
    adios2::adios2
)
