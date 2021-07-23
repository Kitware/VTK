cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(Infovis)

find_package(VTK
  COMPONENTS
    FiltersSources
    IOInfovis
    InfovisCore
    InfovisLayout
    InteractionStyle
    RenderingCore
    RenderingLabel
    RenderingOpenGL2
  OPTIONAL_COMPONENTS
    FiltersStatistics
    GUISupportQt
    IOSQL
    ViewsInfovis
    ViewsQt)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(TreeLayout
  TreeLayout.cxx)
target_link_libraries(TreeLayout
  PRIVATE
    ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS TreeLayout
  MODULES ${VTK_LIBRARIES})

if (TARGET VTK::ViewsInfovis)
  add_executable(HelloWorld
    HelloWorld.cxx)
  target_link_libraries(HelloWorld
    PRIVATE
      ${VTK_LIBRARIES})

  add_executable(Theme
    Theme.cxx)
  target_link_libraries(Theme
    PRIVATE
      ${VTK_LIBRARIES})

  add_executable(CreateTree
    CreateTree.cxx)
  target_link_libraries(CreateTree
    PRIVATE
      ${VTK_LIBRARIES})

  add_executable(MultiView
    MultiView.cxx)
  target_link_libraries(MultiView
    PRIVATE
      ${VTK_LIBRARIES})

  vtk_module_autoinit(
    TARGETS HelloWorld
            Theme
            CreateTree
            MultiView
    MODULES ${VTK_LIBRARIES})
endif ()

# If Qt is enabled include the Qt based examples
if (TARGET VTK::GUISupportQt AND TARGET VTK::ViewsQt)
  add_subdirectory(EasyView)
  add_subdirectory(CustomLinkView)
  if (TARGET VTK::IOSQL AND TARGET VTK::FiltersStatistics)
    add_subdirectory(StatsView)
  endif ()
endif ()
