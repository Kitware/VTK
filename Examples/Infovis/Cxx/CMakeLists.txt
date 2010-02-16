PROJECT(Infovis)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
IF(NOT VTK_USE_INFOVIS)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_INFOVIS.")
ENDIF(NOT VTK_USE_INFOVIS)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

ADD_EXECUTABLE(TreeLayout TreeLayout.cxx)
TARGET_LINK_LIBRARIES(TreeLayout vtkViews vtkInfovis vtkRendering vtkImaging vtkIO)

ADD_EXECUTABLE(HelloWorld HelloWorld.cxx)
TARGET_LINK_LIBRARIES(HelloWorld vtkViews vtkInfovis vtkRendering vtkImaging vtkIO)

ADD_EXECUTABLE(Theme Theme.cxx)
TARGET_LINK_LIBRARIES(Theme vtkViews vtkInfovis vtkRendering vtkImaging vtkIO)

ADD_EXECUTABLE(CreateTree CreateTree.cxx)
TARGET_LINK_LIBRARIES(CreateTree vtkViews vtkInfovis vtkRendering vtkImaging vtkIO)

ADD_EXECUTABLE(MultiView MultiView.cxx)
TARGET_LINK_LIBRARIES(MultiView vtkViews vtkInfovis vtkRendering vtkImaging vtkIO)

# If Qt is enabled include the EasyView example
IF(VTK_USE_QVTK)
  SUBDIRS(ChartView)
  SUBDIRS(EasyView)
  SUBDIRS(CustomLinkView)
  SUBDIRS(StatsView)
ENDIF(VTK_USE_QVTK)

IF(VTK_USE_PARALLEL_BGL)
  ADD_EXECUTABLE(ParallelBFS ParallelBFS.cxx)
  IF (MPI_LIBRARY)
    SET(MPI_LIBRARIES ${MPI_LIBRARY})
  ENDIF (MPI_LIBRARY)
  IF (MPI_EXTRA_LIBRARY)
    SET(MPI_LIBRARIES ${MPI_LIBRARIES} "${MPI_EXTRA_LIBRARY}")
  ENDIF (MPI_EXTRA_LIBRARY)
  TARGET_LINK_LIBRARIES(ParallelBFS vtkParallel vtkViews  ${MPI_LIBRARIES})
ENDIF(VTK_USE_PARALLEL_BGL)
