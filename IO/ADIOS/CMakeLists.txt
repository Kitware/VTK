find_package(ADIOS 1.11 REQUIRED)
include_directories(SYSTEM ${ADIOS_INCLUDE_DIRS})

find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})

find_package (Threads)

set(Module_SOURCES
  ADIOSDefs.cxx
  ADIOSUtilities.cxx

  ADIOSWriter.cxx

  ADIOSAttribute.cxx
  ADIOSVarInfo.cxx
  ADIOSScalar.cxx
  ADIOSReader.cxx

  vtkADIOSUtilities.cxx

  vtkADIOSWriter.cxx

  vtkADIOSDirTree.cxx
  vtkADIOSReader.cxx
  )

set_source_files_properties(
  ADIOSDefs.cxx
  ADIOSUtilities.cxx

  ADIOSWriter.cxx

  ADIOSAttribute.cxx
  ADIOSVarInfo.cxx
  ADIOSScalar.cxx
  ADIOSReader.cxx

  vtkADIOSUtilities.cxx

  vtkADIOSWriterDefine.cxx
  vtkADIOSWriterWrite.cxx

  vtkADIOSDirTree.cxx
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkIOADIOS ${Module_SOURCES})
target_link_libraries(vtkIOADIOS
  LINK_PRIVATE ${ADIOS_LIBRARIES} ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
  )
