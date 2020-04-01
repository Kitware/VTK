cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(Infovis)

set(vtk_modules
find_package(VTK
  COMPONENTS
    FiltersSources
    IOInfovis
    InfovisCore
    InfovisLayout
    InteractionStyle
    RenderingCore
    RenderingLabel
    RenderingOpenGL2)
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
    ${vtk_modules})

vtk_module_autoinit(
  TARGETS TreeLayout
  MODULES ${vtk_modules})

if (TARGET VTK::ViewsInfovis)
  add_executable(HelloWorld
    HelloWorld.cxx)
  target_link_libraries(HelloWorld
    PRIVATE
      ${vtk_modules}
      vtkViewsInfovis)

  add_executable(Theme
    Theme.cxx)
  target_link_libraries(Theme
    PRIVATE
      ${vtk_modules}
      vtkViewsInfovis)

  add_executable(CreateTree
    CreateTree.cxx)
  target_link_libraries(CreateTree
    PRIVATE
      ${vtk_modules}
      vtkViewsInfovis)

  add_executable(MultiView
    MultiView.cxx)
  target_link_libraries(MultiView
    PRIVATE
      ${vtk_modules}
      vtkViewsInfovis)

  vtk_module_autoinit(
    TARGETS HelloWorld
            Theme
            CreateTree
            MultiView
    MODULES ${vtk_modules}
            vtkViewsInfovis)
endif ()

# If Qt is enabled include the Qt based examples
if (TARGET VTK::GUISupportQt AND TARGET VTK::ViewsQt)
  add_subdirectory(EasyView)
  add_subdirectory(CustomLinkView)
  if (TARGET VTK::IOSQL AND TARGET VTK::FiltersStatistics)
    add_subdirectory(StatsView)
  endif ()
endif ()
