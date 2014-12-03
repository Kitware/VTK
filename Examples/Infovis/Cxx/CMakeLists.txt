cmake_minimum_required(VERSION 2.8.8)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

project(Infovis)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

add_executable(TreeLayout TreeLayout.cxx)
target_link_libraries(TreeLayout ${VTK_LIBRARIES})

if(vtkViewsInfovis_LOADED)
  add_executable(HelloWorld HelloWorld.cxx)
  target_link_libraries(HelloWorld ${VTK_LIBRARIES})

  add_executable(Theme Theme.cxx)
  target_link_libraries(Theme ${VTK_LIBRARIES})

  add_executable(CreateTree CreateTree.cxx)
  target_link_libraries(CreateTree ${VTK_LIBRARIES})

  add_executable(MultiView MultiView.cxx)
  target_link_LIBRARIES(MultiView ${VTK_LIBRARIES})
endif()

# If Qt is enabled include the QT based examples
if(vtkGUISupportQt_LOADED)
  add_subdirectory(EasyView)
  add_subdirectory(CustomLinkView)
  add_subdirectory(StatsView)
endif()

if(vtkInfovisParallel_LOADED)
  add_executable(ParallelBFS ParallelBFS.cxx)
  find_package(MPI REQUIRED)
  target_link_libraries(ParallelBFS vtkParallel ${VTK_LIBRARIES} ${MPI_C_LIBRARIES})
  if (MPI_CXX_LIBRARIES)
    target_link_libraries(ParallelBFS ${MPI_CXX_LIBRARIES})
  endif()
endif()
