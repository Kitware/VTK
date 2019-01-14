project(EasyView)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set your files and resources here
set(Srcs
  EasyView.cxx
  main.cxx)
set(UIs
  EasyView.ui)
set(Hdrs
  EasyView.h)
set(Rcs
  Icons/icons.qrc)

# We have ui files, this will bring in the macro: qt5_wrap_ui
find_package(Qt5 COMPONENTS Widgets)
if (NOT TARGET Qt5::Widgets)
  message("Skipping example: ${Qt5_NOT_FOUND_MESSAGE}")
  return ()
endif ()

source_group("Resources" FILES
  ${UIs}
  ${Rcs}
)

add_executable(EasyView
  MACOSX_BUNDLE  ${Srcs} ${Hdrs} ${UIs} ${Rcs})
target_link_libraries(EasyView
  PRIVATE
    ${vtk_modules}
    VTK::GUISupportQt
    VTK::ViewsQt
    Qt5::Widgets)
vtk_module_autoinit(
  TARGETS EasyView
  MODULES ${vtk_modules}
          VTK::GUISupportQt
          VTK::ViewsQt)
