PROJECT(Infovis)

FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
IF(NOT VTK_USE_INFOVIS)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_INFOVIS.")
ENDIF(NOT VTK_USE_INFOVIS)
INCLUDE(${VTK_USE_FILE})

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
