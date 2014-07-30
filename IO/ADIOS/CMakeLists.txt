find_package(ADIOS REQUIRED)
include_directories(SYSTEM ${ADIOS_INCLUDE_DIRS})

find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})

set(Module_SOURCES
  ADIOSDefs.cxx
  ADIOSUtilities.cxx
  ADIOSWriter.cxx
  ADIOSVarInfo.cxx
  ADIOSScalar.cxx
  ADIOSReader.cxx

  vtkADIOSDirTree.cxx
  vtkADIOSReader.cxx
  vtkADIOSWriter.cxx
  vtkADIOSWriterDefine.cxx
  vtkADIOSWriterWrite.cxx
  )

set_source_files_properties(
  ADIOSAttribute
  ADIOSDefs
  ADIOSScalar
  ADIOSUtilities
  ADIOSVarInfo
  ADIOSWriter
  ADIOSReader

  vtkADIOSDirTree
  vtkADIOSWriterDefine
  vtkADIOSWriterWrite
  WRAP_EXCLUDE
  )

vtk_module_library(vtkIOADIOS ${Module_SOURCES})
target_link_libraries(vtkIOADIOS
  LINK_PRIVATE ${ADIOS_LIBRARIES} ${MPI_C_LIBRARIES}
  )
