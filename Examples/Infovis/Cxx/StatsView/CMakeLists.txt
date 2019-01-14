# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set your files and resources here
set(Srcs
  StatsView.cxx
  main.cxx)
set(UIs
  StatsView.ui)
set(Hdrs
  StatsView.h)
set(Rcs
  Icons/icons.qrc)

find_package(Qt5 COMPONENTS Widgets)
if (NOT TARGET Qt5::Widgets)
  message("Skipping example: ${Qt5_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(StatsView MACOSX_BUNDLE
  ${Srcs} ${Hdrs} ${UIs} ${Rcs})
target_link_libraries(StatsView
  PRIVATE
    ${vtk_modules}
    VTK::FiltersStatistics
    VTK::GUISupportQt
    VTK::ViewsQt
    VTK::IOSQL)
vtk_module_autoinit(
  TARGETS StatsView
  MODULES ${vtk_modules}
          VTK::FiltersStatistics
          VTK::GUISupportQt
          VTK::ViewsQt
          VTK::IOSQL)
