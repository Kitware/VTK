# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set your files and resources here
set(Srcs
  CustomLinkView.cxx
  main.cxx)
set(UIs
  CustomLinkView.ui)
set(Hdrs
  CustomLinkView.h)
set(Rcs
  Icons/icons.qrc)

find_package(Qt5 COMPONENTS Widgets)
if (NOT TARGET Qt5::Widgets)
  message("Skipping example: ${Qt5_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(CustomLinkView MACOSX_BUNDLE
  ${Srcs} ${Hdrs} ${UIs} ${Rcs})
target_link_libraries(CustomLinkView
  PRIVATE
    ${vtk_modules}
    VTK::GUISupportQt
    VTK::ViewsQt
    Qt5::Widgets)
vtk_module_autoinit(
  TARGETS CustomLinkView
  MODULES ${vtk_modules}
          VTK::GUISupportQt
          VTK::ViewsQt)
